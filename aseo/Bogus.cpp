#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <cstdlib>
using namespace llvm;
namespace {
    struct Bogus: public FunctionPass{
        static char ID;
        std::vector<BasicBlock*> basicBlocks;
    Bogus() : FunctionPass(ID){};
    virtual bool runOnFunction(Function& F) override{
        bogus(F);
        return true;
    }
    void bogus(Function& F){
        if(F.getName() == "Var2File" || F.getName() == "File2Var"|| F.getName() == "RSHash"
           || F.getName() == "rsa" || F.getName() == "quick_pow")return;
        for(Function::iterator i = F.begin(); i != F.end(); i++){
            basicBlocks.push_back(&*i);
            break;
        }
        int obf_times = 2;
        while(!basicBlocks.empty()){
            BasicBlock* basicBlock = basicBlocks.back();
            addBogusFlow(basicBlock, F);
            basicBlocks.pop_back();
        }
    }
    virtual void addBogusFlow(BasicBlock* basicBlock, Function& F){
        BasicBlock::iterator i1 = basicBlock->begin();
        if(basicBlock->getFirstNonPHIOrDbgOrLifetime()){
            i1 = (BasicBlock::iterator)basicBlock->getFirstNonPHIOrDbgOrLifetime();
        }
        Twine *BBName;
        BBName = new Twine("originBB");
        BasicBlock* originBB = basicBlock->splitBasicBlock(i1, *BBName);
        Twine* alterBBName;
        alterBBName = new Twine("fakeBB");
        //BasicBlock* alterBB = createAlteredBasicBlock(originBB, *alterBBName, &F);
        BasicBlock* alterBB = BasicBlock::Create(F.getContext(),"",&F);
        //BasicBlock* alterBB = BasicBlock::Create(F.getContext(),"fake basic block",&F);
        //alterBB->getTerminator()->eraseFromParent();
        basicBlock->getTerminator()->eraseFromParent();
        Value* LHS = ConstantFP::get(Type::getFloatTy(F.getContext()), 1.0);
        Value* RHS = ConstantFP::get(Type::getFloatTy(F.getContext()), 2.0);
        Twine* var4 = new Twine("conditions");
        FCmpInst* condition = new FCmpInst(*basicBlock, FCmpInst::FCMP_TRUE, LHS, RHS, *var4);

        BranchInst::Create(originBB, alterBB, (Value*)condition, basicBlock);
        if(basicBlocks.size() > 1)
            BranchInst::Create(basicBlocks.at(1 + rand() % (basicBlocks.size()-1)), alterBB);
        else
            BranchInst::Create(originBB, alterBB);

        Twine* var5 = new Twine("originalBBpart2");
        BasicBlock::iterator i = originBB->end();
        BasicBlock *originalBBpart2 = originBB->splitBasicBlock(--i, *var5);
        originBB->getTerminator()->eraseFromParent();
        Twine* var6 = new Twine("condition2");
        FCmpInst *condition2 = new FCmpInst(*originBB, CmpInst::FCMP_TRUE, LHS, RHS, *var6);
        BranchInst::Create(originalBBpart2, alterBB, (Value *)condition2, originBB);
        return;
        doF(*F.getParent());
    }
    virtual BasicBlock *createAlteredBasicBlock(BasicBlock *basicBlock, const Twine &Name = "gen", Function *F = 0) {
            // Useful to remap the informations concerning instructions.
            ValueToValueMapTy VMap;
            BasicBlock *alteredBB = llvm::CloneBasicBlock(basicBlock, VMap, Name, F);
            // Remap operands.
            BasicBlock::iterator ji = basicBlock->begin();
            for (BasicBlock::iterator i = alteredBB->begin(), e = alteredBB->end();
                 i != e; ++i) {
                // Loop over the operands of the instruction
                for (User::op_iterator opi = i->op_begin(), ope = i->op_end(); opi != ope;
                     ++opi) {
                    // get the value for the operand
                    Value *v = MapValue(*opi, VMap, RF_None, 0);
                    if (v != 0) {
                        *opi = v;
                    }
                }
                // Remap phi nodes' incoming blocks.
                if (PHINode *pn = dyn_cast<PHINode>(i)) {
                    for (unsigned j = 0, e = pn->getNumIncomingValues(); j != e; ++j) {
                        Value *v = MapValue(pn->getIncomingBlock(j), VMap, RF_None, 0);
                        if (v != 0) {
                            pn->setIncomingBlock(j, cast<BasicBlock>(v));
                        }
                    }
                }
                // Remap attached metadata.
                SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
                i->getAllMetadata(MDs);
                // important for compiling with DWARF, using option -g.
                i->setDebugLoc(ji->getDebugLoc());
                ji++;

            } // The instructions' informations are now all correct


            // add random instruction in the middle of the bloc. This part can be
            // improve
            for (BasicBlock::iterator i = alteredBB->begin(), e = alteredBB->end();
                 i != e; ++i) {
                // in the case we find binary operator, we modify slightly this part by
                // randomly insert some instructions
                if (i->isBinaryOp()) { // binary instructions
                    unsigned opcode = i->getOpcode();
                    BinaryOperator *op, *op1 = NULL;
                    Twine *var = new Twine("_");
                    // treat differently float or int
                    // Binary int
                    if (opcode == Instruction::Add || opcode == Instruction::Sub ||
                        opcode == Instruction::Mul || opcode == Instruction::UDiv ||
                        opcode == Instruction::SDiv || opcode == Instruction::URem ||
                        opcode == Instruction::SRem || opcode == Instruction::Shl ||
                        opcode == Instruction::LShr || opcode == Instruction::AShr ||
                        opcode == Instruction::And || opcode == Instruction::Or ||
                        opcode == Instruction::Xor) {
                        for (int random = 0; random < 10;
                             ++random) {
                            switch (random % 4) { // to improve
                                case 0:                                    // do nothing
                                    break;
                                case 1:
                                    op = BinaryOperator::CreateNeg(i->getOperand(0), *var, &*i);
                                    op1 = BinaryOperator::Create(Instruction::Add, op,
                                                                 i->getOperand(1), "gen", &*i);
                                    break;
                                case 2:
                                    op1 = BinaryOperator::Create(Instruction::Sub, i->getOperand(0),
                                                                 i->getOperand(1), *var, &*i);
                                    op = BinaryOperator::Create(Instruction::Mul, op1,
                                                                i->getOperand(1), "gen", &*i);
                                    break;
                                case 3:
                                    op = BinaryOperator::Create(Instruction::Shl, i->getOperand(0),
                                                                i->getOperand(1), *var, &*i);
                                    break;
                            }
                        }
                    }
                    // Binary float
                    if (opcode == Instruction::FAdd || opcode == Instruction::FSub ||
                        opcode == Instruction::FMul || opcode == Instruction::FDiv ||
                        opcode == Instruction::FRem) {
                        for (int random = 0; random < 10;
                             ++random) {
                            switch (random % 3) { // can be improved
                                case 0:                                    // do nothing
                                    break;
                                case 1:
                                    op = BinaryOperator::CreateFNeg(i->getOperand(0), *var, &*i);
                                    op1 = BinaryOperator::Create(Instruction::FAdd, op,
                                                                 i->getOperand(1), "gen", &*i);
                                    break;
                                case 2:
                                    op = BinaryOperator::Create(Instruction::FSub, i->getOperand(0),
                                                                i->getOperand(1), *var, &*i);
                                    op1 = BinaryOperator::Create(Instruction::FMul, op,
                                                                 i->getOperand(1), "gen", &*i);
                                    break;
                            }
                        }
                    }
                    if (opcode == Instruction::ICmp) { // Condition (with int)
                        ICmpInst *currentI = (ICmpInst *)(&i);
                        switch (0) { // must be improved
                            case 0:                                    // do nothing
                                break;
                            case 1:
                                currentI->swapOperands();
                                break;
                            case 2: // randomly change the predicate
                                switch (2) {
                                    case 0:
                                        currentI->setPredicate(ICmpInst::ICMP_EQ);
                                        break; // equal
                                    case 1:
                                        currentI->setPredicate(ICmpInst::ICMP_NE);
                                        break; // not equal
                                    case 2:
                                        currentI->setPredicate(ICmpInst::ICMP_UGT);
                                        break; // unsigned greater than
                                    case 3:
                                        currentI->setPredicate(ICmpInst::ICMP_UGE);
                                        break; // unsigned greater or equal
                                    case 4:
                                        currentI->setPredicate(ICmpInst::ICMP_ULT);
                                        break; // unsigned less than
                                    case 5:
                                        currentI->setPredicate(ICmpInst::ICMP_ULE);
                                        break; // unsigned less or equal
                                    case 6:
                                        currentI->setPredicate(ICmpInst::ICMP_SGT);
                                        break; // signed greater than
                                    case 7:
                                        currentI->setPredicate(ICmpInst::ICMP_SGE);
                                        break; // signed greater or equal
                                    case 8:
                                        currentI->setPredicate(ICmpInst::ICMP_SLT);
                                        break; // signed less than
                                    case 9:
                                        currentI->setPredicate(ICmpInst::ICMP_SLE);
                                        break; // signed less or equal
                                }
                                break;
                        }
                    }
                    if (opcode == Instruction::FCmp) { // Conditions (with float)
                        FCmpInst *currentI = (FCmpInst *)(&i);
                        switch (0) { // must be improved
                            case 0:                                    // do nothing
                                break;
                            case 1:
                                currentI->swapOperands();
                                break;
                            case 2: // randomly change the predicate
                                switch (0) {
                                    case 0:
                                        currentI->setPredicate(FCmpInst::FCMP_OEQ);
                                        break; // ordered and equal
                                    case 1:
                                        currentI->setPredicate(FCmpInst::FCMP_ONE);
                                        break; // ordered and operands are unequal
                                    case 2:
                                        currentI->setPredicate(FCmpInst::FCMP_UGT);
                                        break; // unordered or greater than
                                    case 3:
                                        currentI->setPredicate(FCmpInst::FCMP_UGE);
                                        break; // unordered, or greater than, or equal
                                    case 4:
                                        currentI->setPredicate(FCmpInst::FCMP_ULT);
                                        break; // unordered or less than
                                    case 5:
                                        currentI->setPredicate(FCmpInst::FCMP_ULE);
                                        break; // unordered, or less than, or equal
                                    case 6:
                                        currentI->setPredicate(FCmpInst::FCMP_OGT);
                                        break; // ordered and greater than
                                    case 7:
                                        currentI->setPredicate(FCmpInst::FCMP_OGE);
                                        break; // ordered and greater than or equal
                                    case 8:
                                        currentI->setPredicate(FCmpInst::FCMP_OLT);
                                        break; // ordered and less than
                                    case 9:
                                        currentI->setPredicate(FCmpInst::FCMP_OLE);
                                        break; // ordered or less than, or equal
                                }
                                break;
                        }
                    }
                }
            }
            return alteredBB;
        } // end of createAlteredBasicBlock()

    bool doF(Module &M) {
        // replace True with
        std::vector<Instruction *> toEdit, toDelete;
        Twine * varX = new Twine("x");
        Value * x1 =ConstantInt::get(Type::getInt64Ty(M.getContext()), 100, false);
        GlobalVariable 	*x = new GlobalVariable(M, Type::getInt64Ty(M.getContext()), false,
                                                   GlobalValue::ExternalLinkage, (Constant * )x1,
                                                   *varX);
        // Looking for the conditions and branches to transform
        for (Module::iterator mi = M.begin(), me = M.end(); mi != me; ++mi) {
            for (Function::iterator fi = mi->begin(), fe = mi->end(); fi != fe; ++fi) {
                // fi->setName("");
                Instruction *tbb = fi->getTerminator();
                if (tbb->getOpcode() == Instruction::Br) {
                    BranchInst *br = (BranchInst *)(tbb);
                    if (br->isConditional()) {
                        FCmpInst *cond = (FCmpInst *)br->getCondition();
                        unsigned opcode = cond->getOpcode();
                        if (opcode == Instruction::FCmp) {
                            if (cond->getPredicate() == FCmpInst::FCMP_TRUE) {
                                toDelete.push_back(cond); // The condition
                                toEdit.push_back(tbb);    // The branch using the condition
                            }
                        }
                    }
                }
                for(BasicBlock::iterator bi = fi->begin(); bi != fi->end(); bi++) {
                    if(bi->isBinaryOp()) {
                        unsigned opcode = bi->getOpcode();
                        if(opcode == Instruction::Add || opcode == Instruction::Sub || opcode == Instruction::Mul || opcode == Instruction::SDiv){
                            LoadInst* op = new LoadInst ((Value *)x, "", &(*bi));
                            //ConstantInt* CI = dyn_cast<llvm::ConstantInt>(op);

                            BinaryOperator* bop = BinaryOperator::Create(Instruction::Add,
                                                                         (Value*) op,
                                                                         CastInst::CreateIntegerCast(bi->getOperand(0),Type::getInt64Ty(M.getContext()),false,"",&(*bi)),
                                                                         "",
                                                                         &(*bi));
                            StoreInst* sop = new StoreInst((Value*)bop, (Value*)x, &(*bi));
                        }
                    }
                }
            }
        }
        BinaryOperator* op = NULL;
        LoadInst * opX;
        // Replacing all the branches we found
        for (std::vector<Instruction *>::iterator i = toEdit.begin();
            i != toEdit.end(); ++i) {
            opX = new LoadInst ((Value *)x, "", (*i));
            int random = 0 % 4;
            int mod_num[4] = {2,6,24,120};
            op = modCalc(op, x, random + 1, (*i));
            op = BinaryOperator::Create(Instruction::SRem,
                                        op,
                                        ConstantInt::get(Type::getInt64Ty(M.getContext()), mod_num[random], false),
                                        "",
                                        (*i));
            ICmpInst *condition = new ICmpInst(
                        (*i), ICmpInst::ICMP_EQ, op,
                        ConstantInt::get(Type::getInt64Ty(M.getContext()), 0, false));
            BranchInst::Create(((BranchInst *)*i)->getSuccessor(0),
                                   ((BranchInst *)*i)->getSuccessor(1), (Value *)condition,
                                   ((BranchInst *)*i)->getParent());
            (*i)->eraseFromParent(); // erase the branch
        }
            // Erase all the associated conditions we found
            for (std::vector<Instruction *>::iterator i = toDelete.begin();
                 i != toDelete.end(); ++i) {
                (*i)->eraseFromParent();
            }
            return true;
        }


        BinaryOperator *modCalc(BinaryOperator *op,  GlobalVariable *x, int flag, Instruction *i);
    };
};
BinaryOperator* Bogus::modCalc(BinaryOperator* op,  GlobalVariable *x, int flag, Instruction *i){
    if(flag < 0) {
        return op;
    }
    if(op == NULL){
        op = BinaryOperator::Create(Instruction::Mul,
                                    ConstantInt::get(Type::getInt64Ty(i->getContext()), 1, false),
                                    ConstantInt::get(Type::getInt64Ty(i->getContext()), 1, false),
                                    "",
                                    i);
    }
    LoadInst* opX = new LoadInst ((Value *)x, "", i);
    BinaryOperator* tmp = BinaryOperator::Create(Instruction::Add,
                                                 ConstantInt::get(Type::getInt64Ty(i->getContext()), flag, false),
                                                 (Value *)opX,
                                                 "",
                                                 i);
    op = BinaryOperator::Create(Instruction::Mul, op, tmp, "", i);
    return modCalc(op, x, --flag, i);
}
char Bogus::ID = 0;
static RegisterPass<Bogus> Y("bogus", "bogus control flow");