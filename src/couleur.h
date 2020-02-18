#ifndef COULEUR
#define COULEUR

#include <iostream>
#include <complex>
#include <iostream>

using namespace std;

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

void black(){ printf("%s", KNRM); }
void red  (const char* txt){ printf("%s%s%s", KRED, txt, KNRM); }
void green(const char* txt){ printf("%s%s%s", KGRN, txt, KNRM); }


#endif




/*


FFFFFF  L       OOOOOO  RRRRRR  IIIIII  AAAAAA  N    N     L       OOOOOO  U    U  PPPPPP  IIIIII  AAAAAA    SSSS
F       L       O    O  R    R    II    A    A  NN   N     L       O    O  U    U  P    P    II    A    A  SS
FFF     L       O    O  RRRRR     II    AAAAAA  N  N N     L       O    O  U    U  PPPPPP    II    AAAAAA   SSSS
F       L       O    O  R  R      II    A    A  N   NN     L       O    O  U    U  P         II    A    A      SS
F       LLLLLL  OOOOOO  R   RR  IIIIII  A    A  N    N     LLLLLL  OOOOOO  UUUUUU  P       IIIIII  A    A  SSSS



*/