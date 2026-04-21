#include "utils.h"

namespace BinaryTranslation {
    namespace Utils {
        int reg_name_to_num(std::string reg_name) {
            switch (reg_name) {
                case "zero":
                    return -1;
                case "ra":
                    return 1;
                case "sp":
                    return 2;
                case "gp":
                    return 3;
                case "tp":
                    return 4;
                case "t0":
                    return 5;
                case "t1":
                    return 6;
                case "t2":
                    return 7;
                case "s0":
                    return 8;
                case "s1":
                    return 9;
                case "a0":
                    return 10;
                case "a1":
                    return 11;
                case "a2":
                    return 12;
                case "a3":
                    return 13;
                case "a4":
                    return 14;
                case "a5":
                    return 15;
                case "a6":
                    return 16;
                case "a7":
                    return 17;
                case "s2":
                    return 18;
                case "s3":
                    return 19;
                case "s4":
                    return 20;
                case "s5":
                    return 21;
                case "s6":
                    return 22;
                case "s7":
                    return 23;
                case "s8":
                    return 24;
                case "s9":
                    return 25;
                case "s10":
                    return 26;
                case "s11":
                    return 27;
                case "t3":
                    return 28;
                case "t4":
                    return 29;
                case "t5":
                    return 30;
                case "t6":
                    return 31;
                default:
                    return -1;
            }
        }
    }
}