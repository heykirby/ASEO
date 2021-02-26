#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;
using namespace std;
#include<stdio.h>
typedef int64_t ll;
void Var2File(ll x) {
    FILE *fp = NULL;
    fp = fopen("ASEO_DATA.txt", "w");
    fprintf(fp, "%d", x^0xA4E0);
    fclose(fp);
}
ll File2Var() {
    FILE *fp = NULL;
    char buff[255];
    fp = fopen("ASEO_DATA.txt", "r");
    fscanf(fp, "%s", buff);
    fclose(fp);
    ll x = atoi(buff);
    return x;
}
namespace {
  struct StoreVar : public FunctionPass {
      static char ID;
      StoreVar() : FunctionPass(ID){};
      virtual bool runOnFunction(Function &F) override {
          Module& M = *F.getParent();
          FunctionCallee V2F = M.getOrInsertFunction("Var2File",
                                                     Type::getVoidTy(M.getContext()),
                                                     Type::getInt64Ty(M.getContext()));

          FunctionCallee F2V = M.getOrInsertFunction("File2Var",
                                                     Type::getInt64Ty(M.getContext()));
          std::vector<BasicBlock*> basicBlocks;
          for(Function::iterator fi = F.begin(); fi != F.end(); fi++){
              if(F.getName() == "Var2File" || F.getName() == "File2Var"|| F.getName() == "RSHash"
              || F.getName() == "rsa" || F.getName() == "quick_pow")
                  break;
              basicBlocks.push_back(&(*fi));
          }

          for(std::vector<BasicBlock*>::iterator fi = basicBlocks.begin(); fi != basicBlocks.end(); fi++) {
              for(BasicBlock::iterator bi = (*fi)->begin(); bi != (*fi)->end(); bi++) {
                  if(bi->isBinaryOp()) {
                      unsigned opcode = bi->getOpcode();
                      if (opcode == Instruction::Add || opcode == Instruction::Sub ||
                          opcode == Instruction::Mul || opcode == Instruction::UDiv ||
                          opcode == Instruction::SDiv || opcode == Instruction::URem ||
                          opcode == Instruction::SRem || opcode == Instruction::Shl ||
                          opcode == Instruction::LShr || opcode == Instruction::AShr ||
                          opcode == Instruction::And || opcode == Instruction::Or ||
                          opcode == Instruction::Xor)  {
                          Value* left = bi->getOperand(0);
                          Value* right = bi->getOperand(1);
                          left = CastInst::CreateIntegerCast(left,Type::getInt64Ty(M.getContext()), false,"",&*bi);
                          right = CastInst::CreateIntegerCast(right,Type::getInt64Ty(M.getContext()), false,"",&*bi);
                          BasicBlock* newBB = (*fi)->splitBasicBlock(++bi, "new bb");
                          Instruction *bt = (*fi)->getTerminator();
                          std::vector<Value*> argsLeft,argsRight;
                          CallInst *callInstLeft, *callInstRight;
                          argsLeft.push_back(left);
                          CallInst::Create(V2F, argsLeft, "", bt);
                          callInstLeft = CallInst::Create(F2V,"",bt);
                          argsRight.push_back(right);
                          CallInst::Create(V2F, argsRight, "", bt);
                          callInstRight = CallInst::Create(F2V,"",bt);
                          BinaryOperator* op = BinaryOperator::Create(static_cast<Instruction::BinaryOps>(opcode), callInstLeft, callInstRight, "", bt);
                          bt->replaceAllUsesWith(op);
                          //bi->dropAllReferences();
                          //bi->eraseFromParent();
                          ICmpInst* leftCond = new ICmpInst((*fi)->getTerminator(), ICmpInst::ICMP_EQ, left, callInstLeft);
                          ICmpInst* rightCond = new ICmpInst((*fi)->getTerminator(),ICmpInst::ICMP_EQ, right, callInstRight);
                          ICmpInst* bothCond = new ICmpInst((*fi)->getTerminator(), ICmpInst::ICMP_EQ, leftCond, rightCond);
                          ICmpInst* trueExp = new ICmpInst((*fi)->getTerminator(), ICmpInst::ICMP_NE,
                                                           ConstantInt::get(Type::getInt64Ty(M.getContext()), 0, false),
                                                           ConstantInt::get(Type::getInt64Ty(M.getContext()), 1, false));
                          bothCond = new ICmpInst((*fi)->getTerminator(), ICmpInst::ICMP_EQ, bothCond, trueExp);
                          BasicBlock* loopBB = BasicBlock::Create((*fi)->getContext(), "loop bb", &F);
                          BranchInst::Create(loopBB,loopBB);
                          BranchInst::Create(newBB, loopBB, bothCond, (*fi)->getTerminator());
                          (*fi)->getTerminator()->eraseFromParent();
                          return true;
                      }
                  }
              }
          }
          return true;
      };
  };
};
char StoreVar::ID = 0;
static RegisterPass<StoreVar> A("store", "store var to file and read from file");