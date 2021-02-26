#include <iterator>
#include <string>

#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

namespace
{

    using namespace llvm;
    class FunctionInstCounter : public FunctionPass
    {
    public:
        static char ID;
        FunctionInstCounter(): FunctionPass(ID){}
        bool runOnFunction(Function& func) override
        {
            if (func.getName() != "test")
            {
                return false;
            }

            unsigned int instCount = 0;
            for (BasicBlock& bb : func)
            {
                instCount += std::distance(bb.begin(), bb.end());
            }

            llvm::outs() << "Number of instructions in " << func.getName() << ": "
                         << instCount << "\n";
            return false;
        }
    };

} // namespace

char FunctionInstCounter::ID = 0;
static RegisterPass<FunctionInstCounter> Z("instCount", "count inst");