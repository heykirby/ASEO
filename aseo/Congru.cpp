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
ll quick_pow(ll n){
    ll x = 2;
    ll m = 10000229;
    ll res = 1;
    while(n > 0){
        if(n & 1)res = (res * x) % m;
        x = (x * x) % m;
        n >>= 1;
    }
    return res;
}
using namespace llvm;
namespace {
    struct Congru : public FunctionPass {
        static char ID;
        Congru() : FunctionPass(ID) {};
        virtual bool runOnFunction(Function &F) override {
            Module& M = *F.getParent();
            FunctionCallee pow_func = M.getOrInsertFunction("quick_pow",
                                                             Type::getInt64Ty(M.getContext()),
                                                             Type::getInt64Ty(M.getContext()));
            std::vector<BasicBlock*> basicBlocks;
            for(Function::iterator fi = F.begin(); fi != F.end(); fi++){
                //if(F.getName() != "test")break;
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
                            BasicBlock* leftB = term->getSuccessor(0);
                            BasicBlock* rightB = term->getSuccessor(1);
                            if(dyn_cast<PHINode>(leftB->begin()) || dyn_cast<PHINode>(rightB->begin())) {
                                continue;
                            }
                            CmpInst::Predicate predicate = cond->getPredicate();
                            if(predicate == ICmpInst::ICMP_EQ
                            //|| predicate == ICmpInst::ICMP_NE
                            ) {
                                std::vector<Value*> args;
                                Value* left = cond->getOperand(0);
                                Value* right = cond->getOperand(1);
                                Value* var;
                                Value* real;
                                int a;
                                CallInst* callInst;
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
                                    }
                                    a = dyn_cast<ConstantInt>(real)->getSExtValue();
                                    if(a >= 10000229)continue;
                                    callInst = CallInst::Create(pow_func,args,"",term);
                                    ICmpInst* range_cond = new ICmpInst(term,CmpInst::ICMP_ULT,var,ConstantInt::get(Type::getInt64Ty(M.getContext()), 10000229, false));
                                    BasicBlock *originBB = BasicBlock::Create((*fi)->getContext(), "", &F);
                                    ICmpInst* new_cond = new ICmpInst(*originBB, predicate, callInst, ConstantInt::get(Type::getInt64Ty(term->getContext()), quick_pow(a), false));
                                    if(predicate == ICmpInst::ICMP_EQ)
                                        BranchInst::Create(originBB, rightB, range_cond, term);
                                    else
                                        BranchInst::Create(originBB, leftB, range_cond, term);
                                    BranchInst::Create(leftB, rightB, new_cond, originBB);
                                    cond->eraseFromParent();
                                    term->eraseFromParent();
                                    return true;
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
char Congru::ID = 0;
static RegisterPass<Congru> Z("congru", "replace var with congru(var) on if branch");