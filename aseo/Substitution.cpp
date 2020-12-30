#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include <cstdlib>
using namespace llvm;
namespace {
    struct Substitution : public FunctionPass {
        static char ID;
        void (Substitution::*funcAdd[4])(BinaryOperator *bo);
        void (Substitution::*funcSub[3])(BinaryOperator *bo);
        void (Substitution::*funcAnd[2])(BinaryOperator *bo);
        void (Substitution::*funcOr[2])(BinaryOperator *bo);
        void (Substitution::*funcXor[2])(BinaryOperator *bo);

        Substitution() : FunctionPass(ID) {
            funcAdd[0] = &Substitution::addNeg;
            funcAdd[1] = &Substitution::addDoubleNeg;
            funcAdd[2] = &Substitution::addRand;
            funcAdd[3] = &Substitution::addRand2;

            funcSub[0] = &Substitution::subNeg;
            funcSub[1] = &Substitution::subRand;
            funcSub[2] = &Substitution::subRand2;

            funcAnd[0] = &Substitution::andSubstitution;
            funcAnd[1] = &Substitution::andSubstitutionRand;

            funcOr[0] = &Substitution::orSubstitution;
            funcOr[1] = &Substitution::orSubstitutionRand;

            funcXor[0] = &Substitution::xorSubstitution;
            funcXor[1] = &Substitution::xorSubstitutionRand;
        };
        bool runOnFunction(Function &F);

        void addNeg(BinaryOperator *bo);
        void addDoubleNeg(BinaryOperator *bo);
        void addRand(BinaryOperator *bo);
        void addRand2(BinaryOperator *bo);

        void subNeg(BinaryOperator *bo);
        void subRand(BinaryOperator *bo);
        void subRand2(BinaryOperator *bo);

        void andSubstitution(BinaryOperator *bo);
        void andSubstitutionRand(BinaryOperator *bo);

        void orSubstitution(BinaryOperator *bo);
        void orSubstitutionRand(BinaryOperator *bo);

        void xorSubstitution(BinaryOperator *bo);
        void xorSubstitutionRand(BinaryOperator *bo);
    };
}

char Substitution::ID = 0;
static RegisterPass<Substitution> X("substitution", "operators substitution");

bool Substitution::runOnFunction(Function &F) {
    int addKey, subKey, andKey, orKey, xorKey;
    for(int times = 10; times >= 0; times--){
        for(Function::iterator bb = F.begin(); bb != F.end(); bb++) {
            for(BasicBlock:: iterator inst = bb->begin(); inst != bb->end(); inst++) {
                if(inst->isBinaryOp()) {
                    switch (inst->getOpcode()) {
                        case BinaryOperator::Add:
                            (this->*funcAdd[addKey % 4])(cast<BinaryOperator>(inst));
                            ++addKey;
                            break;
                        /*case BinaryOperator::Sub:
                            (this->*funcSub[subKey % 3])(cast<BinaryOperator>(inst));
                            ++subKey;
                            break;
                        case BinaryOperator::And:
                            (this->*funcAnd[andKey % 2])(cast<BinaryOperator>(inst));
                            ++andKey;
                            break;
                        case BinaryOperator::Or:
                            (this->*funcOr[orKey % 2])(cast<BinaryOperator>(inst));
                            ++orKey;
                            break;
                        case BinaryOperator::Xor:
                            (this->*funcXor[xorKey % 2])(cast<BinaryOperator>(inst));
                            ++xorKey;
                            break;*/
                        default:
                            break;
                    }
                }
            }
        }
    }
    return false;
}

// Implementation of a = b - (-c)
void Substitution::addNeg(BinaryOperator *bo) {
    BinaryOperator *op = NULL;
    BinaryOperator *op_srem = NULL;

    if (bo->getOpcode() == Instruction::Add) {
        op = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
        op = BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), op, "", bo);

        Type* ty = bo->getType();
        ConstantInt *one = (ConstantInt *)ConstantInt::get(ty, 1);
        ConstantInt *two = (ConstantInt *)ConstantInt::get(ty, 2);
        op_srem = BinaryOperator::Create(Instruction::Add,bo->getOperand(0), one, "", bo);
        op_srem = BinaryOperator::Create(Instruction::Mul, op_srem, bo->getOperand(0), "", bo);
        op_srem = BinaryOperator::Create(Instruction::SRem, op_srem, two, "", bo);
        op = BinaryOperator::Create(Instruction::Add, op, op_srem, "", bo);
        bo->replaceAllUsesWith(op);
    }
}

// Implementation of a = -(-b + (-c))
void Substitution::addDoubleNeg(BinaryOperator *bo) {
    BinaryOperator *op, *op2 = NULL;
    BinaryOperator *op_srem = NULL;
    if (bo->getOpcode() == Instruction::Add) {
        op = BinaryOperator::CreateNeg(bo->getOperand(0), "", bo);
        op2 = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
        op = BinaryOperator::Create(Instruction::Add, op, op2, "", bo);
        op = BinaryOperator::CreateNeg(op, "", bo);

        Type* ty = bo->getType();
        ConstantInt *one = (ConstantInt *)ConstantInt::get(ty, 1);
        ConstantInt *two = (ConstantInt *)ConstantInt::get(ty, 2);
        op_srem = BinaryOperator::Create(Instruction::Add,bo->getOperand(0), one, "", bo);
        op_srem = BinaryOperator::Create(Instruction::Mul, op_srem, bo->getOperand(0), "", bo);
        op_srem = BinaryOperator::Create(Instruction::SRem, op_srem, two, "", bo);
        op = BinaryOperator::Create(Instruction::Add, op, op_srem, "", bo);
        bo->replaceAllUsesWith(op);
    }
}

// Implementation of  r = rand (); a = b + r; a = a + c; a = a - r
void Substitution::addRand(BinaryOperator *bo) {
    BinaryOperator *op = NULL;
    BinaryOperator *op_srem = NULL;
    if (bo->getOpcode() == Instruction::Add) {
        Type *ty = bo->getType();
        ConstantInt *co = (ConstantInt *)ConstantInt::get(ty, rand());
        op = BinaryOperator::Create(Instruction::Add, bo->getOperand(0), co, "", bo);
        op = BinaryOperator::Create(Instruction::Add, op, bo->getOperand(1), "", bo);
        op = BinaryOperator::Create(Instruction::Sub, op, co, "", bo);

        ConstantInt *one = (ConstantInt *)ConstantInt::get(ty, 1);
        ConstantInt *two = (ConstantInt *)ConstantInt::get(ty, 2);
        op_srem = BinaryOperator::Create(Instruction::Add,bo->getOperand(0), one, "", bo);
        op_srem = BinaryOperator::Create(Instruction::Mul, op_srem, bo->getOperand(0), "", bo);
        op_srem = BinaryOperator::Create(Instruction::SRem, op_srem, two, "", bo);
        op = BinaryOperator::Create(Instruction::Add, op, op_srem, "", bo);
        bo->replaceAllUsesWith(op);
    }
}

// Implementation of r = rand (); a = b - r; a = a + b; a = a + r
void Substitution::addRand2(BinaryOperator *bo) {
    BinaryOperator *op = NULL;
    BinaryOperator *op_srem = NULL;
    if (bo->getOpcode() == Instruction::Add) {
        Type *ty = bo->getType();
        ConstantInt *co = (ConstantInt *)ConstantInt::get(ty, rand());
        op = BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), co, "", bo);
        op = BinaryOperator::Create(Instruction::Add, op, bo->getOperand(1), "", bo);
        op = BinaryOperator::Create(Instruction::Add, op, co, "", bo);

        ConstantInt *one = (ConstantInt *)ConstantInt::get(ty, 1);
        ConstantInt *two = (ConstantInt *)ConstantInt::get(ty, 2);
        op_srem = BinaryOperator::Create(Instruction::Add,bo->getOperand(0), one, "", bo);
        op_srem = BinaryOperator::Create(Instruction::Mul, op_srem, bo->getOperand(0), "", bo);
        op_srem = BinaryOperator::Create(Instruction::SRem, op_srem, two, "", bo);
        op = BinaryOperator::Create(Instruction::Add, op, op_srem, "", bo);
        bo->replaceAllUsesWith(op);
    }
}

// Implementation of a = b + (-c)
void Substitution::subNeg(BinaryOperator *bo) {
    BinaryOperator *op = NULL;

    if (bo->getOpcode() == Instruction::Sub) {
        op = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
        op =
                BinaryOperator::Create(Instruction::Add, bo->getOperand(0), op, "", bo);

    } else {
        op = BinaryOperator::CreateFNeg(bo->getOperand(1), "", bo);
        op = BinaryOperator::Create(Instruction::FAdd, bo->getOperand(0), op, "",
                                    bo);
    }

    bo->replaceAllUsesWith(op);
}

// Implementation of  r = rand (); a = b + r; a = a - c; a = a - r
void Substitution::subRand(BinaryOperator *bo) {
    BinaryOperator *op = NULL;

    if (bo->getOpcode() == Instruction::Sub) {
        Type *ty = bo->getType();
        ConstantInt *co =
                (ConstantInt *)ConstantInt::get(ty, rand());
        op =
                BinaryOperator::Create(Instruction::Add, bo->getOperand(0), co, "", bo);
        op =
                BinaryOperator::Create(Instruction::Sub, op, bo->getOperand(1), "", bo);
        op = BinaryOperator::Create(Instruction::Sub, op, co, "", bo);

        // Check signed wrap
        // op->setHasNoSignedWrap(bo->hasNoSignedWrap());
        // op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

        bo->replaceAllUsesWith(op);
    }
    /* else {
        Type *ty = bo->getType();
        ConstantFP *co =
    (ConstantFP*)ConstantFP::get(ty,(float)llvm::cryptoutils->get_uint64_t());
        op = BinaryOperator::Create(Instruction::FAdd,bo->getOperand(0),co,"",bo);
        op = BinaryOperator::Create(Instruction::FSub,op,bo->getOperand(1),"",bo);
        op = BinaryOperator::Create(Instruction::FSub,op,co,"",bo);
    } */
}

// Implementation of  r = rand (); a = b - r; a = a - c; a = a + r
void Substitution::subRand2(BinaryOperator *bo) {
    BinaryOperator *op = NULL;

    if (bo->getOpcode() == Instruction::Sub) {
        Type *ty = bo->getType();
        ConstantInt *co =
                (ConstantInt *)ConstantInt::get(ty, rand());
        op =
                BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), co, "", bo);
        op =
                BinaryOperator::Create(Instruction::Sub, op, bo->getOperand(1), "", bo);
        op = BinaryOperator::Create(Instruction::Add, op, co, "", bo);

        // Check signed wrap
        // op->setHasNoSignedWrap(bo->hasNoSignedWrap());
        // op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

        bo->replaceAllUsesWith(op);
    }
    /* else {
        Type *ty = bo->getType();
        ConstantFP *co =
    (ConstantFP*)ConstantFP::get(ty,(float)llvm::cryptoutils->get_uint64_t());
        op = BinaryOperator::Create(Instruction::FSub,bo->getOperand(0),co,"",bo);
        op = BinaryOperator::Create(Instruction::FSub,op,bo->getOperand(1),"",bo);
        op = BinaryOperator::Create(Instruction::FAdd,op,co,"",bo);
    } */
}

// Implementation of a = b & c => a = (b^~c)& b
void Substitution::andSubstitution(BinaryOperator *bo) {
    BinaryOperator *op = NULL;

    // Create NOT on second operand => ~c
    op = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

    // Create XOR => (b^~c)
    BinaryOperator *op1 =
            BinaryOperator::Create(Instruction::Xor, bo->getOperand(0), op, "", bo);

    // Create AND => (b^~c) & b
    op = BinaryOperator::Create(Instruction::And, op1, bo->getOperand(0), "", bo);
    bo->replaceAllUsesWith(op);
}

// Implementation of a = a && b <=> !(!a | !b) && (r | !r)
void Substitution::andSubstitutionRand(BinaryOperator *bo) {
    // Copy of the BinaryOperator type to create the random number with the
    // same type of the operands
    Type *ty = bo->getType();

    // r (Random number)
    ConstantInt *co =
            (ConstantInt *)ConstantInt::get(ty, rand());

    // !a
    BinaryOperator *op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo);

    // !b
    BinaryOperator *op1 = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

    // !r
    BinaryOperator *opr = BinaryOperator::CreateNot(co, "", bo);

    // (!a | !b)
    BinaryOperator *opa =
            BinaryOperator::Create(Instruction::Or, op, op1, "", bo);

    // (r | !r)
    opr = BinaryOperator::Create(Instruction::Or, co, opr, "", bo);

    // !(!a | !b)
    op = BinaryOperator::CreateNot(opa, "", bo);

    // !(!a | !b) && (r | !r)
    op = BinaryOperator::Create(Instruction::And, op, opr, "", bo);

    // We replace all the old AND operators with the new one transformed
    bo->replaceAllUsesWith(op);
}

// Implementation of a = b | c => a = (b & c) | (b ^ c)
void Substitution::orSubstitutionRand(BinaryOperator *bo) {

    Type *ty = bo->getType();
    ConstantInt *co =
            (ConstantInt *)ConstantInt::get(ty, rand());

    // !a
    BinaryOperator *op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo);

    // !b
    BinaryOperator *op1 = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

    // !r
    BinaryOperator *op2 = BinaryOperator::CreateNot(co, "", bo);

    // !a && r
    BinaryOperator *op3 =
            BinaryOperator::Create(Instruction::And, op, co, "", bo);

    // a && !r
    BinaryOperator *op4 =
            BinaryOperator::Create(Instruction::And, bo->getOperand(0), op2, "", bo);

    // !b && r
    BinaryOperator *op5 =
            BinaryOperator::Create(Instruction::And, op1, co, "", bo);

    // b && !r
    BinaryOperator *op6 =
            BinaryOperator::Create(Instruction::And, bo->getOperand(1), op2, "", bo);

    // (!a && r) || (a && !r)
    op3 = BinaryOperator::Create(Instruction::Or, op3, op4, "", bo);

    // (!b && r) ||(b && !r)
    op4 = BinaryOperator::Create(Instruction::Or, op5, op6, "", bo);

    // (!a && r) || (a && !r) ^ (!b && r) ||(b && !r)
    op5 = BinaryOperator::Create(Instruction::Xor, op3, op4, "", bo);

    // !a || !b
    op3 = BinaryOperator::Create(Instruction::Or, op, op1, "", bo);

    // !(!a || !b)
    op3 = BinaryOperator::CreateNot(op3, "", bo);

    // r || !r
    op4 = BinaryOperator::Create(Instruction::Or, co, op2, "", bo);

    // !(!a || !b) && (r || !r)
    op4 = BinaryOperator::Create(Instruction::And, op3, op4, "", bo);

    // [(!a && r) || (a && !r) ^ (!b && r) ||(b && !r) ] || [!(!a || !b) && (r ||
    // !r)]
    op = BinaryOperator::Create(Instruction::Or, op5, op4, "", bo);
    bo->replaceAllUsesWith(op);
}

void Substitution::orSubstitution(BinaryOperator *bo) {
    BinaryOperator *op = NULL;

    // Creating first operand (b & c)
    op = BinaryOperator::Create(Instruction::And, bo->getOperand(0),
                                bo->getOperand(1), "", bo);

    // Creating second operand (b ^ c)
    BinaryOperator *op1 = BinaryOperator::Create(
            Instruction::Xor, bo->getOperand(0), bo->getOperand(1), "", bo);

    // final op
    op = BinaryOperator::Create(Instruction::Or, op, op1, "", bo);
    bo->replaceAllUsesWith(op);
}

// Implementation of a = a ~ b => a = (!a && b) || (a && !b)
void Substitution::xorSubstitution(BinaryOperator *bo) {
    BinaryOperator *op = NULL;

    // Create NOT on first operand
    op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo); // !a

    // Create AND
    op = BinaryOperator::Create(Instruction::And, bo->getOperand(1), op, "",
                                bo); // !a && b

    // Create NOT on second operand
    BinaryOperator *op1 =
            BinaryOperator::CreateNot(bo->getOperand(1), "", bo); // !b

    // Create AND
    op1 = BinaryOperator::Create(Instruction::And, bo->getOperand(0), op1, "",
                                 bo); // a && !b

    // Create OR
    op = BinaryOperator::Create(Instruction::Or, op, op1, "",
                                bo); // (!a && b) || (a && !b)
    bo->replaceAllUsesWith(op);
}

// implementation of a = a ^ b <=> (a ^ r) ^ (b ^ r) <=> (!a && r || a && !r) ^
// (!b && r || b && !r)
// note : r is a random number
void Substitution::xorSubstitutionRand(BinaryOperator *bo) {
    BinaryOperator *op = NULL;

    Type *ty = bo->getType();
    ConstantInt *co =
            (ConstantInt *)ConstantInt::get(ty, rand());

    // !a
    op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo);

    // !a && r
    op = BinaryOperator::Create(Instruction::And, co, op, "", bo);

    // !r
    BinaryOperator *opr = BinaryOperator::CreateNot(co, "", bo);

    // a && !r
    BinaryOperator *op1 =
            BinaryOperator::Create(Instruction::And, bo->getOperand(0), opr, "", bo);

    // !b
    BinaryOperator *op2 = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

    // !b && r
    op2 = BinaryOperator::Create(Instruction::And, op2, co, "", bo);

    // b && !r
    BinaryOperator *op3 =
            BinaryOperator::Create(Instruction::And, bo->getOperand(1), opr, "", bo);

    // (!a && r) || (a && !r)
    op = BinaryOperator::Create(Instruction::Or, op, op1, "", bo);

    // (!b && r) || (b && !r)
    op1 = BinaryOperator::Create(Instruction::Or, op2, op3, "", bo);

    // (!a && r) || (a && !r) ^ (!b && r) || (b && !r)
    op = BinaryOperator::Create(Instruction::Xor, op, op1, "", bo);
    bo->replaceAllUsesWith(op);
}
