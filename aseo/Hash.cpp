#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include<stdio.h>
#include<stdlib.h>
 typedef int64_t ll;
 ll RSHash(ll x)
{
    char *str = (char*)calloc(10, sizeof(char));
    sprintf(str,"%d",x);
    ll b = 378551;
    ll a = 63689;
    ll hash = 0;
    while (*str)
    {
        hash = hash * a + *str++;
        a *= b;
    }
    return (hash & 0x7FFFFFFF);
}

using namespace llvm;
namespace {
  struct Hash : public FunctionPass {
      static char ID;
      Hash() : FunctionPass(ID) {};
      virtual bool runOnFunction(Function &F) override {
          Module& M = *F.getParent();
          FunctionCallee hash_func = M.getOrInsertFunction("RSHash",
                                                           Type::getInt64Ty(M.getContext()),
                                                           Type::getInt64Ty(M.getContext()));
          std::vector<BasicBlock*> basicBlocks;
          for(Function::iterator fi = F.begin(); fi != F.end(); fi++){
              if(F.getName() == "Var2File" || F.getName() == "File2Var"|| F.getName() == "RSHash"
                 || F.getName() == "rsa" || F.getName() == "quick_pow")
                  break;
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
                          if(predicate == ICmpInst::ICMP_EQ) {
                              std::vector<Value*> args;
                              Value* left = cond->getOperand(0);
                              Value* right = cond->getOperand(1);
                              int real;
                              CallInst* callInst;
                              if(dyn_cast<ConstantInt>(left) != dyn_cast<ConstantInt>(right)) {
                                  if(dyn_cast<ConstantInt>(left)) {
                                      real = RSHash(dyn_cast<ConstantInt>(left)->getSExtValue());
                                      left = CastInst::CreateIntegerCast(left,Type::getInt64Ty(M.getContext()), false,"",term);
                                      right = CastInst::CreateIntegerCast(right,Type::getInt64Ty(M.getContext()), false,"",term);
                                      args.push_back(right);
                                  }
                                  if(dyn_cast<ConstantInt>(right)) {
                                      real = RSHash(dyn_cast<ConstantInt>(right)->getSExtValue());
                                      left = CastInst::CreateIntegerCast(left,Type::getInt64Ty(M.getContext()), false,"",term);
                                      right = CastInst::CreateIntegerCast(right,Type::getInt64Ty(M.getContext()), false,"",term);
                                      args.push_back(left);
                                  }
                                  //leftB->setName("left branch");
                                  //rightB->setName("right branch");
                                  callInst = CallInst::Create(hash_func,args,"",term);
                                  ICmpInst* new_cond = new ICmpInst(term, predicate, callInst, ConstantInt::get(Type::getInt64Ty(term->getContext()), real, false));
                                  BasicBlock* originBB = BasicBlock::Create((*fi)->getContext(), "origin", &F);
                                  ICmpInst* old_cond = new ICmpInst(*originBB, predicate, left, right);
                                  BranchInst::Create(leftB, rightB, old_cond, originBB);
                                  BranchInst::Create(originBB, rightB, new_cond, term);
                                  term->eraseFromParent();
                                  return true;
                              }
                          }
                          else if(predicate == ICmpInst::ICMP_NE) {
                              continue;
                              std::vector<Value *> args;
                              Value *left = cond->getOperand(0);
                              Value *right = cond->getOperand(1);
                              int real;
                              CallInst *callInst;
                              if (dyn_cast<ConstantInt>(left) != dyn_cast<ConstantInt>(right)) {
                                  if (dyn_cast<ConstantInt>(left)) {
                                      real = RSHash(dyn_cast<ConstantInt>(left)->getSExtValue());
                                      args.push_back(right);
                                  }
                                  if (dyn_cast<ConstantInt>(right)) {
                                      real = RSHash(dyn_cast<ConstantInt>(right)->getSExtValue());
                                      args.push_back(left);
                                  }
                                  BasicBlock *leftB = term->getSuccessor(0);
                                  BasicBlock *rightB = term->getSuccessor(1);
                                  //leftB->setName("left branch");
                                  //rightB->setName("right branch");
                                  callInst = CallInst::Create(hash_func, args, "", term);
                                  ICmpInst *new_cond = new ICmpInst(term, predicate, callInst, ConstantInt::get(
                                          Type::getInt32Ty(term->getContext()), real, false));
                                  BasicBlock *originBB = BasicBlock::Create((*fi)->getContext(), "origin", &F);
                                  ICmpInst *old_cond = new ICmpInst(*originBB, predicate, left, right);
                                  BranchInst::Create(leftB, rightB, old_cond, originBB);
                                  BranchInst::Create(leftB, originBB, new_cond, term);
                                  term->eraseFromParent();
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
char Hash::ID = 0;
static RegisterPass<Hash> Z("hash", "replace var with hash(var) on if branch");