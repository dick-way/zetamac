#include <sys/timers.h>
#include <ti/getcsc.h>
#include <ti/screen.h>
#include <ti/real.h>
#include <graphx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <keypadc.h>
#include <math.h>
#include <time.h>

/*  arithmetic operation type          */

const char ADD = '+';
const char SUB = '-';

/*  addition/subtraction               */

const int ADDMIN = 20;
const int ADDMAX = 100;

/*  multiplication/division            */

const int MULTMIN = 2;
const int MULTMAX = 14;

/*  factorial                          */

const int FACTMIN = 3;
const int FACTMAX = 6;

/*  addition/subtraction fractions     */

const int DENMIN = 1;
const int DENMAX = 12;

const int NUMMIN = 1;
const int NUMMAX = 11;

int factorsSize;

/*  exponentiation/root                */

const int BASEMIN = 2;
const int BASEMAX = 7;
const double BASEWEIGHTS[6] = {0.48, 0.16, 0.13, 0.13, 0.05, 0.05};

const int EXPMIN = 3;
const int EXPMAX[6] = {8, 4, 4, 4, 3, 3};

const double PROBLEMWEIGHTS[9] = {0.11, 0.11, 0.11, 0.11, 0.11, 0.11, 0.11, 0.11, 0.11};  /*  TODO: Weighted probabilities, customization  */

const char* character0 = "0";
const char* character1 = "1";
const char* character2 = "2";
const char* character3 = "3";
const char* character4 = "4";
const char* character5 = "5";
const char* character6 = "6";
const char* character7 = "7";
const char* character8 = "8";
const char* character9 = "9";
const char* characterS = "/";
const char* characterM = "-";

char problem[17];

char answer[7];
char guess[7];
int score = 0;

char scoreDisp[4];

int spacingX = 4;
int spacingY = 4;

kb_key_t key1, key2, key3, key4, key5;

int inputLock = 0;

float timer_seconds = 90.0f;
float difference;

char timerDisp[10];
clock_t start;

uint8_t header = 0x4A;

int digitShift = 8;

int randIntWeighted(int min, int max, const double weight[]) {

    double r = (double) rand() / RAND_MAX;
    double cumulative = 0.0;
    int range = max - min + 1;

    for(int i = 0; i < range; i++) {
    
        cumulative += weight[i];

        if(r <= cumulative) {

            return min + i;

        }

    }

    return max;

}

int randIntFraction(int arr[], int size) {

    return arr[randInt(0, size - 1)];

}

int powInt(int base, int expo) {

    int product = 1;

    for(int i = 0; i < expo; i++) {

        product *= base;

    }

    return product;

}

int* appendItem(int arr[], int size, int item) {

    int* appended_arr = malloc((size + 1) * sizeof(int));

    if(appended_arr == NULL) {

        return NULL;

    }

    for(int i = 0; i < size; i++) {

        appended_arr[i] = arr[i];

    }

    appended_arr[size] = item;

    return appended_arr;

}

void appendChar(const char *item, int index) {

    int i = 0;

    while(item[i] != '\0') {

        guess[index + i] = item[i];
        i++;

    }

    guess[index + i] = '\0';

}

void printCentered(const char *str, int y) {

    gfx_SetTextFGColor(0x00);
    gfx_SetTextBGColor(0xFF);
    gfx_PrintStringXY(str, (GFX_LCD_WIDTH - gfx_GetStringWidth(str)) / 2, y);

}

static void printTime(float elapsed) {

    real_t elapsed_real;

    elapsed_real = os_FloatToReal(elapsed <= 0.001f ? 0.0f : elapsed);

    os_RealToStr(timerDisp, &elapsed_real, 8, 1, 2);

    gfx_SetTextFGColor(0xFF);
    gfx_SetTextBGColor(header);

    if(difference < 10.00) {

        gfx_PrintStringXY("0", 6, 6);
        
        gfx_SetColor(header);
        gfx_FillRectangle(38, 6, 10, 8);

    }

    gfx_PrintStringXY(timerDisp, (difference < 10.00) ? 6 + digitShift : 6, 6);

}

void redrawScreen() {

    gfx_SetColor(0xFF);
    gfx_FillRectangle((GFX_LCD_WIDTH / 2) - 48, 80, 96, 8);

    printCentered(problem, 80);

    snprintf(scoreDisp, sizeof(scoreDisp), "%i", score);

    gfx_SetTextFGColor(0xFF);
    gfx_SetTextBGColor(header);
    gfx_PrintStringXY(scoreDisp, GFX_LCD_WIDTH - 6 - (8 * strlen(scoreDisp)), 6);

    gfx_SetTextFGColor(0x00);
    gfx_SetTextBGColor(0xFF);
    gfx_PrintStringXY("V 0.01-beta", 6, GFX_LCD_HEIGHT - 14);

}

void appendGuess(const char *item) {

    if(inputLock == 0) {

        inputLock = 1;

        int length = (int) strlen(guess);

        if(length < 6) {

            appendChar(item, length);

        }

    }

}

void clearGuess() {

    if(inputLock == 0) {

        gfx_SetColor(0xFF);
        gfx_FillRectangle((GFX_LCD_WIDTH / 2) - (48 / 2), 120, 48, 8);

        guess[0] = '\0';

        inputLock = 1;

    }

}

void textEntry() {

    do {

        kb_Scan();

        key1 = kb_Data[3];
        key2 = kb_Data[4];
        key3 = kb_Data[5];
        key4 = kb_Data[6];
        key5 = kb_Data[1];

        if(key1 == kb_0) {

            appendGuess(character0);

        } else if (key1 == kb_1) {

            appendGuess(character1);

        } else if (key1 == kb_4) {

            appendGuess(character4);

        } else if (key1 == kb_7) {

            appendGuess(character7);

        } else if (key2 == kb_2) {

            appendGuess(character2);

        } else if (key2 == kb_5) {

            appendGuess(character5);

        } else if (key2 == kb_8) {

            appendGuess(character8);

        } else if (key3 == kb_3) {

            appendGuess(character3);

        } else if (key3 == kb_6) {

            appendGuess(character6);

        } else if (key3 == kb_9) {

            appendGuess(character9);

        } else if (key3 == kb_Chs) {

            appendGuess(characterM);

        } else if (key4 == kb_Enter) {

            if(inputLock == 0 && guess[0] != '\0') {
            
                break;

                inputLock = 1;

            }

        } else if (key4 == kb_Clear) {

            clearGuess();

        } else if (key4 == kb_Div) {

            appendGuess(characterS);

        } else if (key5 == kb_Mode) {

        /*  Force quit  */

            gfx_End();
            exit(0);

        } else {

            inputLock = 0;

        }

        gfx_SetTextFGColor(0x00);
        gfx_SetTextBGColor(0xFF);
        gfx_PrintStringXY(guess, (GFX_LCD_WIDTH / 2) - (48 / 2), 120);

    /*  Calculate and print the elapsed time  */
        clock_t now = clock();
        float elapsed = (float)(now - start) / CLOCKS_PER_SEC;

        difference = timer_seconds - elapsed;
        printTime(difference);

    } while (difference > 0);

}

int* fairFactors(int term1Den) {

    int* factors;
    int size = 0;

    if(1 < term1Den && term1Den < 6 && randInt(0, 1) == 1) {

        for(int i = 2; i < 6; i++) {

            if(term1Den != i && (i % term1Den != 0 || term1Den % i != 0)) {

                factors = appendItem(factors, size, i);
                size++;

            }

        }

    } else {

        for(int i = DENMIN; i < term1Den; i++) {

            if(term1Den % i == 0) {

                factors = appendItem(factors, size, i);
                size++;

            }

        }

        for(int i = term1Den + 1; i < DENMAX; i++) {

            if(i % term1Den == 0) {

                factors = appendItem(factors, size, i);
                size++;

            }

        }
    
    }

    factorsSize = size;

    return factors;

}

int gcd(int a, int b) {

    return b == 0 ? a : gcd(b, a % b);

}

int lcm(int a, int b) {

    return a * b / gcd(a, b);

}

void arithmetic(const char operation) {

    int term1 = randInt(ADDMIN, ADDMAX);
    int term2 = randInt(ADDMIN, ADDMAX);

    snprintf(problem, sizeof(problem), "%i %c %i", term1, operation, term2);

    snprintf(answer, sizeof(answer), "%d", (operation == ADD) ? term1 + term2 : term1 - term2);

}

void multiplication() {

    int term1 = randInt(MULTMIN, MULTMAX);
    int term2 = randInt(MULTMIN, MULTMAX);

    snprintf(problem, sizeof(problem), "%i * %i", term1, term2);

    snprintf(answer, sizeof(answer), "%d", term1 * term2);

}

void division() {

    int term1 = randInt(MULTMIN, MULTMAX);
    int term2 = randInt(MULTMIN, MULTMAX);

    snprintf(problem, sizeof(problem), "%i / %i", term1 * term2, term1);

    snprintf(answer, sizeof(answer), "%d", term2);

}

void factorial() {

    int term1 = randInt(FACTMIN, FACTMAX);
    int product = 1;

    snprintf(problem, sizeof(problem), "%i!", term1);

    for(int i = 2; i < term1 + 1; i++) {

        product *= i;

    }

    snprintf(answer, sizeof(answer), "%d", product);

}

void arithmeticFraction(const char operation) {

    int term1Num = randInt(NUMMIN, NUMMAX);
    int term1Den = randInt(DENMIN, DENMAX);

    while(gcd(term1Num, term1Den) > 1) {

        term1Num++;

    }

    int* factors = fairFactors(term1Den);

    int term2Num = randInt(NUMMIN, NUMMAX);
    int term2Den = randIntFraction(factors, factorsSize);

    while(gcd(term2Num, term2Den) > 1) {

        term2Num++;

    }

    if(term1Den == 1 && term2Den == 1) {

        snprintf(problem, sizeof(problem), "%i %c %i", term1Num, operation, term2Num);

    } else if(term1Den == 1) {

        snprintf(problem, sizeof(problem), "%i %c %i/%i", term1Num, operation, term2Num, term2Den);

    } else if (term2Den == 1) {

        snprintf(problem, sizeof(problem), "%i/%i %c %i", term1Num, term1Den, operation, term2Num);

    } else {

        snprintf(problem, sizeof(problem), "%i/%i %c %i/%i", term1Num, term1Den, operation, term2Num, term2Den);

    }

    int denominator = lcm(term1Den, term2Den);
    int scaled1Num = term1Num * (denominator / term1Den);
    int scaled2Num = term2Num * (denominator / term2Den);
    int numerator = (operation == ADD) ? scaled1Num + scaled2Num : scaled1Num - scaled2Num;

    int gcdivisor = gcd(abs(numerator), denominator);
    numerator /= gcdivisor;
    denominator /= gcdivisor;

    if(denominator == 1) {

        snprintf(answer, sizeof(answer), "%d", numerator);

    } else {
        
        snprintf(answer, sizeof(answer), "%d/%d", numerator, denominator);

    }

    free(factors);

}

void exponentiation() {

    int term1 = randIntWeighted(BASEMIN, BASEMAX, BASEWEIGHTS);
    int term2 = randInt(3, EXPMAX[term1 - BASEMIN]);

    snprintf(problem, sizeof(problem), "%i^%i", term1, term2);

    snprintf(answer, sizeof(answer), "%i", powInt(term1, term2));

}

void exponentiationInverse() {

    int term1 = randIntWeighted(BASEMIN, BASEMAX, BASEWEIGHTS);
    int term2 = randInt(3, EXPMAX[term1 - BASEMIN]);

    snprintf(problem, sizeof(problem), "%i^(1/%i)", powInt(term1, term2), term2);

    snprintf(answer, sizeof(answer), "%d", term1);

}

void generateProblem() {

    int problemType = randInt(0, 8);

    switch(problemType) {

        case 0:
            arithmetic(ADD);
            break;

        case 1:
            arithmetic(SUB);
            break;

        case 2:
            multiplication();
            break;

        case 3:
            division();
            break;

        case 4:
            factorial();
            break;

        case 5:
            arithmeticFraction(ADD);
            break;

        case 6:
            arithmeticFraction(SUB);
            break;

        case 7:
            exponentiation();
            break;

        case 8:
            exponentiationInverse();
            break;

    }

}

int main(void) {

    srand(time(NULL));
    
/*  Initialize graphics drawing  */
    gfx_Begin();

    gfx_SetTextTransparentColor(0xDF);

/*  Draw screen layout  */
    gfx_FillScreen(0xFF);
    
    gfx_SetColor(0x00);
    gfx_Rectangle((GFX_LCD_WIDTH / 2) - (48 / 2) - spacingX, 120 - spacingY, 48 + (spacingX * 2), 8 + (spacingY * 2));

    gfx_SetColor(header);
    gfx_FillRectangle(0, 0, GFX_LCD_WIDTH, 20);

    start = clock();

    do {

    /*  Generate new problem, print to screen  */
        problem[0] = '\0';
        generateProblem();

        redrawScreen();

        textEntry();

        if(strcmp(guess, answer) == 0) {

            score++;

        }

        clearGuess();

    } while(difference > 0);

/*  Reset screen, final display  */
    gfx_FillScreen(0xFF);
    printCentered("program completed", 80);

    printCentered(scoreDisp, 120);

/*  Waits for a key  */
    while (!os_GetCSC());

/*  End graphics drawing  */
    gfx_End();

    return 0;

}