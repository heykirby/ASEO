#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include<stdio.h>
#include<stdlib.h>
typedef int64_t ll;
// p = 10000229; x = 2
ll rsa(ll m){
    //ll p = 10000121;
    //ll q = 10000141;
    //ll n = p * q;
    //ll f = (p - 1) * (q - 1) + 1;
    ll f, n;
    f = n = 10000229;
    ll res = 1;
    while(f > 0){
        if(f & 1)res = (res * m) % n;
        m = (m * m) % n;
        f >>= 1;
    }
    return res;
}
using namespace llvm;
namespace {
    struct RSA : public FunctionPass {
        static char ID;
        RSA() : FunctionPass(ID) {};
        virtual bool runOnFunction(Function &F) override {
            Module& M = *F.getParent();
            FunctionCallee rsa_func = M.getOrInsertFunction("rsa",
                                                            Type::getInt64Ty(M.getContext()),
                                                            Type::getInt64Ty(M.getContext()));
            std::vector<BasicBlock*> basicBlocks;
            for(Function::iterator fi = F.begin(); fi != F.end(); fi++){
                if(F.getName() == "Var2File" || F.getName() == "File2Var"|| F.getName() == "RSHash"
                   || F.getName() == "rsa" || F.getName() == "quick_pow")break;
                basicBlocks.push_back(&(*fi));
            }
            for(std::vector<BasicBlock*>::iterator fi = basicBlocks.begin(); fi != basicBlocks.end(); fi++) {
                Instruction *term = (*fi)->getTerminator();
                if(term->getOpcode() == Instruction::Br) {
                    BranchInst* br = (BranchInst *)term;
                    if(br->isConditional()) {
                        ICmpInst* cond = (ICmpInst*) br->getCondition();
                        if(cond->getOpcode() == ICmpInst::ICmp) {
                            CmpInst::Predicate predicate = cond->getPredicate();
                            BasicBlock* leftB = term->getSuccessor(0);
                            BasicBlock* rightB = term->getSuccessor(1);
                            if(dyn_cast<PHINode>(leftB->begin()) || dyn_cast<PHINode>(rightB->begin())) {
                                continue;
                            }
                            if(predicate == ICmpInst::ICMP_SLT || predicate == ICmpInst::ICMP_SGT) {
                                std::vector<Value*> args;
                                Value* left = cond->getOperand(0);
                                Value* right = cond->getOperand(1);
                                Value* var;
                                Value* real;
                                int a;
                                CallInst* callInst;
                                bool flag = true;
                                if(dyn_cast<ConstantInt>(left) != dyn_cast<ConstantInt>(right)) {
                                    if(dyn_cast<ConstantInt>(left)) {
                                        real = left;
                                        right = CastInst::CreateIntegerCast(right,Type::getInt64Ty(M.getContext()), false,"",term);
                                        args.push_back(right);
                                        var = right;
                                    }
                                    if(dyn_cast<ConstantInt>(right)) {
                                        real = right;
                                        left = CastInst::CreateIntegerCast(left,Type::getInt64Ty(M.getContext()), false,"",term);
                                        args.push_back(left);
                                        var = left;
                                        flag = false;
                                    }
                                    a = dyn_cast<ConstantInt>(real)->getSExtValue();
                                    callInst = CallInst::Create(rsa_func,args,"",term);
                                    ICmpInst* range_cond = new ICmpInst(term,CmpInst::ICMP_SLT,var,ConstantInt::get(Type::getInt64Ty(M.getContext()), 10000229, false));
                                    BasicBlock *originBB = BasicBlock::Create((*fi)->getContext(), "", &F);
                                    if(predicate == ICmpInst::ICMP_SLT) {
                                        BranchInst::Create(originBB, rightB, range_cond, term);
                                    } else {
                                        BranchInst::Create(originBB, leftB, range_cond, term);
                                    }
                                    ICmpInst* new_cond;
                                    if(flag) {
                                        new_cond = new ICmpInst(*originBB, predicate, ConstantInt::get(Type::getInt64Ty(term->getContext()), a, false), callInst);
                                    } else {
                                        new_cond = new ICmpInst(*originBB, predicate, callInst, ConstantInt::get(Type::getInt64Ty(term->getContext()), a, false));
                                    }
                                    BranchInst::Create(leftB, rightB, new_cond, originBB);
                                    cond->eraseFromParent();
                                    term->eraseFromParent();
                                    //return true;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }
    };
};
char RSA::ID = 0;
static RegisterPass<RSA> Z("rsa", "rsa");