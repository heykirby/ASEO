#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include<stdio.h>
#include<stdlib.h>
unsigned int RSHash(int x)
{
    char *str = (char*)calloc(10, sizeof(char));
    sprintf(str,"%d",x);
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    while (*str)
    {
        hash = hash * a + *str++;
        a *= b;
    }
    return (hash & 0x7FFFFFFF);
}
using namespace llvm;
namespace {
  struct CallFunc : public FunctionPass {
      static char ID;
      CallFunc() : FunctionPass(ID) {};
      virtual bool runOnFunction(Function &F) override {
          Module& M = *F.getParent();
          FunctionCallee hash_func = M.getOrInsertFunction("RSHash",
                                                           Type::getInt32Ty(M.getContext()),
                                                           Type::getInt32Ty(M.getContext()));
          std::vector<BasicBlock*> basicBlocks;
          for(Function::iterator fi = F.begin(); fi != F.end(); fi++){
              basicBlocks.push_back(&(*fi));
          }
          for(std::vector<BasicBlock*>::iterator fi = basicBlocks.begin(); fi != basicBlocks.end(); fi++) {
              Instruction *term = (*fi)->getTerminator();
              if(term->getOpcode() == Instruction::Br) {
                  BranchInst* br = (BranchInst *)term;
                  if(br->isConditional()) {
                      ICmpInst* cond = (ICmpInst*) br->getCondition();
                      if(cond->getOpcode() == ICmpInst::ICmp) {
                          if(cond->getPredicate() == ICmpInst::ICMP_EQ) {
                              std::vector<Value*> args;
                              Value* left = cond->getOperand(0);
                              Value* right = cond->getOperand(1);
                              int real;
                              CallInst* callInst;
                              if(dyn_cast<ConstantInt>(left) != dyn_cast<ConstantInt>(right)) {
                                  if(dyn_cast<ConstantInt>(left)) {
                                      real = RSHash(dyn_cast<ConstantInt>(left)->getSExtValue());
                                      args.push_back(right);
                                  }
                                  if(dyn_cast<ConstantInt>(right)) {
                                      errs()<<111<<"\n";
                                      real = RSHash(dyn_cast<ConstantInt>(right)->getSExtValue());
                                      args.push_back(left);
                                  }
                                  BasicBlock* leftB = term->getSuccessor(0);
                                  BasicBlock* rightB = term->getSuccessor(1);
                                  leftB->setName("left branch");
                                  rightB->setName("right branch");
                                  callInst = CallInst::Create(hash_func,args,"",term);
                                  ICmpInst* new_cond = new ICmpInst(term, ICmpInst::ICMP_EQ, callInst, ConstantInt::get(Type::getInt32Ty(term->getContext()), real, false));
                                  BasicBlock* originBB = BasicBlock::Create((*fi)->getContext(), "origin", &F);
                                  ICmpInst* old_cond = new ICmpInst(*originBB, ICmpInst::ICMP_EQ, left, right);
                                  BranchInst::Create(leftB, rightB, old_cond, originBB);
                                  BranchInst::Create(originBB, rightB, new_cond, term);
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
char CallFunc::ID = 0;
static RegisterPass<CallFunc> Z("hash", "call function");