#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;
namespace {
  struct Explode : public FunctionPass {
      static char ID;
      static const int level = 1000;
      Explode() : FunctionPass(ID){};
      virtual bool runOnFunction(Function& F) override{
          std::vector<BasicBlock*> basicBlocks;
          for(Function::iterator fi = F.begin(); fi != F.end(); fi++){
              basicBlocks.push_back(&*fi);
          }
          for(std::vector<BasicBlock*>::iterator fi = basicBlocks.begin(); fi != basicBlocks.end(); fi++) {
              Instruction* term = (*fi)->getTerminator();
              if(term->getOpcode() != Instruction::Br) continue;
              BranchInst* br = (BranchInst*) term;
              if(!br->isConditional()) continue;
              CmpInst* cond = (CmpInst*) br->getCondition();
              if(cond->getOpcode() != ICmpInst::ICmp) continue;
              CmpInst::Predicate predicate = cond->getPredicate();
              BasicBlock* leftB = term->getSuccessor(0);
              BasicBlock* rightB = term->getSuccessor(1);
              Value* left = cond->getOperand(0);
              Value* right = cond->getOperand(1);
              if(predicate == ICmpInst::ICMP_EQ || predicate == CmpInst::ICMP_SLE || predicate == CmpInst::ICMP_SLT) {
                  BasicBlock* newB = BasicBlock::Create(F.getContext(), "base block", &F);
                  ICmpInst* loop_cond = new ICmpInst(term, predicate, left, right);
                  BranchInst::Create(leftB, newB, loop_cond, term);
                  term->eraseFromParent();
                  int tmp = level;
                  while(--tmp > 0) {
                      BasicBlock* nextB = BasicBlock::Create(F.getContext(), "next block", &F);
                      BinaryOperator* bp = BinaryOperator::Create(Instruction::Add, right,
                                                                  ConstantInt::get(Type::getInt32Ty(F.getContext()), tmp, false),
                                                                  "", newB);
                      loop_cond = new ICmpInst(*newB, predicate, left, bp, "");
                      BranchInst::Create(nextB, nextB, loop_cond, newB);
                      newB = nextB;
                  }
                  BranchInst::Create(rightB, newB);
              } else if(predicate == CmpInst::ICMP_SGE || predicate == CmpInst::ICMP_SGT) {
                  BasicBlock* newB = BasicBlock::Create(F.getContext(), "base block", &F);
                  ICmpInst* loop_cond = new ICmpInst(term, predicate, left, right);
                  BranchInst::Create(leftB, newB, loop_cond, term);
                  term->eraseFromParent();
                  int tmp = level;
                  while(--tmp >= 0) {
                      BasicBlock* nextB = BasicBlock::Create(F.getContext(), "next block", &F);
                      BinaryOperator* bp = BinaryOperator::Create(Instruction::Sub, right,
                                                                  ConstantInt::get(Type::getInt32Ty(F.getContext()), tmp, false),
                                                                  "", newB);
                      loop_cond = new ICmpInst(*newB, predicate, left, bp, "");
                      BranchInst::Create(nextB, nextB, loop_cond, newB);
                      newB = nextB;
                  }
                  BranchInst::Create(rightB, newB);
              }
          }

              return true;
      }
  };

};
char Explode::ID = 0;
static RegisterPass<Explode> Z("ex", "explode one branch to much");