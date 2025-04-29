#include <sys/timers.h>
#include <ti/getcsc.h>
#include <ti/screen.h>
#include <ti/real.h>
#include <graphx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <keypadc.h>
#include <fileioc.h>
#include <math.h>
#include <time.h>

#define PTYPEMEM 9

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

int ptypeProblems[PTYPEMEM];
double ptypeWeights[PTYPEMEM];  /*  TODO: Weighted probabilities, customization  */
int ptypeState[PTYPEMEM];

int ptypeSize = 9;
int ptypeSelection = 9;

const char *ptypeName[] = {"Addition", "Subtraction", "Multiplication", "Division", "Factorials", "Addition - Fraction", "Subtraction - Fraction", "Exponentiation", "Exponentiation - Inverse"};

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

kb_key_t key1, key3, key4, key5, key6, key7;
int inputLock[19];

char problem[17];
char answer[7];
char guess[7];

char scoreDisp[4];
int score = 0;

const int spacingX = 4;  /*  Clean up UI code  */
const int spacingY = 4;

float timer_seconds = 90.0f;
float difference;

clock_t start;
char timerDisp[10];
int digitShift = 8;

uint8_t header = 0x4A;

uint8_t var;
const char* appvar = "Zetamac";

/*  TODO: "Redid config menu, save settings"  */

int randIntWeighted(int min, int max, const double weight[]) {

    double r = (double) rand() / RAND_MAX;
    double cumulative = 0.0;
    int range = max - min + 1;

    for (int i = 0; i < range; i++) {
    
        cumulative += weight[i];

        if(r <= cumulative) {

            return min + i;

        }

    }

    return max;

}

int randIntArray(int arr[], int size) {

    return arr[randInt(0, size - 1)];

}

bool ptypeAllOff() {

    for (int i = 0; i < PTYPEMEM; i++) {

        if (ptypeState[i] == 1) {

            return false;

        }

    }

    return true;

}

void forceQuit(int success) {

    gfx_FillScreen(0x00);
    gfx_End();

    exit(success);

}

int powInt(int base, int expo) {

    int product = 1;

    for (int i = 0; i < expo; i++) {

        product *= base;

    }

    return product;

}

int* appendItem(int arr[], int size, int item) {  /*  TODO: linked list, no malloc  */

    int* appended_arr = malloc((size + 1) * sizeof(int));

    if (appended_arr == NULL) {

        return NULL;

    }

    for (int i = 0; i < size; i++) {

        appended_arr[i] = arr[i];

    }

    appended_arr[size] = item;

    return appended_arr;

}

void appendChar(const char *item, int index) {

    int i = 0;

    while (item[i] != '\0') {

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

    printCentered(problem, 80);  /*  TODO: LaTeX-type display for problem  */

    snprintf(scoreDisp, sizeof(scoreDisp), "%i", score);  /*  TODO: local top 5 leader board  */

    gfx_SetTextFGColor(0xFF);
    gfx_SetTextBGColor(header);
    gfx_PrintStringXY(scoreDisp, GFX_LCD_WIDTH - 6 - (8 * strlen(scoreDisp)), 6);

    gfx_SetTextFGColor(0x00);
    gfx_SetTextBGColor(0xFF);
    gfx_PrintStringXY("V 1.02", 6, GFX_LCD_HEIGHT - 14);

}

void appendGuess(const char *item) {

    int length = (int) strlen(guess);

    if (length < 6) {

        appendChar(item, length);

    }

}

void clearGuess() {

    gfx_SetColor(0xFF);
    gfx_FillRectangle((GFX_LCD_WIDTH / 2) - (48 / 2), 120, 48, 8);

    guess[0] = '\0';

}

void textEntry() {  /*  TODO: fix this fucking piece of shit  */

    do {

        kb_Scan();

        key1 = kb_Data[1];
        key3 = kb_Data[3];
        key4 = kb_Data[4];
        key5 = kb_Data[5];
        key6 = kb_Data[6];

        if (key1 & kb_Mode) {

            forceQuit(0);

        }
        
        if (key3 & kb_0 && inputLock[0] == 0) {

            appendGuess(character0);

            inputLock[0] = 1;

        } else if (!(key3 & kb_0)) {

            inputLock[0] = 0;

        }
        
        if (key3 & kb_1 && inputLock[1] == 0) {

            appendGuess(character1);

            inputLock[1] = 1;

        } else if (!(key3 & kb_1)) {

            inputLock[1] = 0;

        }
        
        if (key3 & kb_4 && inputLock[4] == 0) {

            appendGuess(character4);

            inputLock[4] = 1;

        } else if (!(key3 & kb_4)) {

            inputLock[4] = 0;

        }
        
        if (key3 & kb_7 && inputLock[7] == 0) {

            appendGuess(character7);

            inputLock[7] = 1;

        } else if (!(key3 & kb_7)) {

            inputLock[7] = 0;

        }
        
        if (key4 & kb_2 && inputLock[2] == 0) {

            appendGuess(character2);

            inputLock[2] = 1;

        } else if (!(key4 & kb_2)) {

            inputLock[2] = 0;

        }
        
        if (key4 & kb_5 && inputLock[5] == 0) {

            appendGuess(character5);

            inputLock[5] = 1;

        } else if (!(key4 & kb_5)) {

            inputLock[5] = 0;

        }
        
        if (key4 & kb_8 && inputLock[8] == 0) {

            appendGuess(character8);

            inputLock[8] = 1;

        } else if (!(key4 & kb_8)) {

            inputLock[8] = 0;

        }
        
        if (key5 & kb_3 && inputLock[3] == 0) {

            appendGuess(character3);

            inputLock[3] = 1;

        } else if (!(key5 & kb_3)) {

            inputLock[3] = 0;

        }
        
        if (key5 & kb_6 && inputLock[6] == 0) {

            appendGuess(character6);

            inputLock[6] = 1;

        } else if (!(key5 & kb_6)) {

            inputLock[6] = 0;

        }
        
        if (key5 & kb_9 && inputLock[9] == 0) {

            appendGuess(character9);

            inputLock[9] = 1;

        } else if (!(key5 & kb_9)) {

            inputLock[9] = 0;

        }
        
        if (key5 & kb_Chs && inputLock[11] == 0) {

            appendGuess(characterM);

            inputLock[11] = 1;

        } else if (!(key5 & kb_Chs)) {

            inputLock[11] = 0;

        }
        
        if (key6 & kb_Enter && inputLock[18] == 0 && guess[0] != '\0') {

            inputLock[18] = 1;

            break;

        } else if (!(key6 & kb_Enter)) {

            inputLock[18] = 0;

        }
        
        if (key6 & kb_Clear && inputLock[16] == 0) {

            clearGuess();

            inputLock[16] = 1;

        } else if (!(key6 & kb_Clear)) {

            inputLock[16] = 0;

        }
        
        if (key6 & kb_Div && inputLock[10] == 0) {

            appendGuess(characterS);

            inputLock[10] = 1;

        } else if (!(key6 & kb_Div)) {

            inputLock[10] = 0;

        }

        gfx_SetTextFGColor(0x00);
        gfx_SetTextBGColor(0xFF);
        gfx_PrintStringXY(guess, (GFX_LCD_WIDTH / 2) - (48 / 2), 120);

        clock_t now = clock();
        float elapsed = (float)(now - start) / CLOCKS_PER_SEC;

        difference = timer_seconds - elapsed;
        printTime(difference);

    } while (difference > 0);

}

int ptypeRemoveItem(int index) {

    if (index < 0 || index >= ptypeSize) {

        return 0;

    }

    for (int i = index; i < ptypeSize - 1; i++) {

        ptypeProblems[i] = ptypeProblems[i + 1];
        ptypeState[i] = ptypeState[i + 1];

    }

    ptypeSize--;

    return 1;

}

void configSelection(int color, int selection) {

    if(ptypeSelection < 9) {

        int x0 = (GFX_LCD_WIDTH / 2) - (gfx_GetStringWidth(ptypeName[selection]) / 2);
        int y0 = (selection * 12);

        int x1 = (GFX_LCD_WIDTH / 2) + (gfx_GetStringWidth(ptypeName[selection]) / 2);

        gfx_SetColor(color);
        gfx_FillTriangle((x0 - 10), y0 + 46, (x0 - 10), y0 + 42, (x0 - 6), y0 + 44);
        gfx_FillTriangle((x1 + 6), y0 + 46, (x1 + 6), y0 + 42, (x1 + 2), y0 + 44);

    }

    if(ptypeSelection == 9) {

        gfx_SetColor(0x00);
        gfx_Rectangle((GFX_LCD_WIDTH / 2) - 32, 176, 64, 16);

    } else {

        gfx_SetColor(0xFF);
        gfx_Rectangle((GFX_LCD_WIDTH / 2) - 32, 176, 64, 16);

    }

}

void redrawConfigMenu() {

    for (int i = 0; i < PTYPEMEM; i++) {

        printCentered(ptypeName[i], 40 + (i * 12));

        if (ptypeState[i] == 0) {

            gfx_SetColor(0x00);
            gfx_Line((GFX_LCD_WIDTH / 2) - (gfx_GetStringWidth(ptypeName[i]) / 2), (i * 12) + 44, (GFX_LCD_WIDTH / 2) + (gfx_GetStringWidth(ptypeName[i]) / 2), (i * 12) + 44);

        }

    }

    printCentered("Start", 180);

    configSelection(0x00, ptypeSelection);

}

void configMenu() {

    do {

        kb_Scan();

        key6 = kb_Data[6];
        key7 = kb_Data[7];

        if (key6 & kb_Enter && inputLock[18] == 0) {

            inputLock[18] = 1;

            if(ptypeSelection < 9) {

                ptypeState[ptypeSelection] = (ptypeState[ptypeSelection] == 1) ? 0 : 1;

                redrawConfigMenu();

            } else {

                break;

            }

        } else if (!(key6 & kb_Enter)) {

            inputLock[18] = 0;

        }

        if (key7 & kb_Up && inputLock[12] == 0) {

            configSelection(0xFF, ptypeSelection);

            if (ptypeSelection > 0) {

                ptypeSelection--;

            } else {

                ptypeSelection = 8;

            }

            redrawConfigMenu();

            inputLock[12] = 1;

        } else if (!(key7 & kb_Up)) {

            inputLock[12] = 0;

        }

        if (key7 & kb_Down && inputLock[13] == 0) {

            configSelection(0xFF, ptypeSelection);

            if (ptypeSelection < 9) {

                ptypeSelection++;

            } else {

                ptypeSelection = 0;

            }

            redrawConfigMenu();

            inputLock[13] = 1;

        } else if (!(key7 & kb_Down)) {

            inputLock[13] = 0;

        }

    } while(1);

}

int* fairFactors(int term1Den) {  /*  TODO: point bonus for arithmetic fractions  */

    int* factors;
    int size = 0;

    if (1 < term1Den && term1Den < 6 && randInt(0, 1) == 1) {

        for (int i = 2; i < 6; i++) {

            if (term1Den != i && (i % term1Den != 0 || term1Den % i != 0)) {

                factors = appendItem(factors, size, i);
                size++;

            }

        }

    } else {

        for (int i = DENMIN; i < term1Den; i++) {

            if (term1Den % i == 0) {

                factors = appendItem(factors, size, i);
                size++;

            }

        }

        for (int i = term1Den + 1; i < DENMAX; i++) {

            if (i % term1Den == 0) {

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

    for (int i = 2; i < term1 + 1; i++) {

        product *= i;

    }

    snprintf(answer, sizeof(answer), "%d", product);

}

void arithmeticFraction(const char operation) {

    int term1Num = randInt(NUMMIN, NUMMAX);
    int term1Den = randInt(DENMIN, DENMAX);

    while (gcd(term1Num, term1Den) > 1) {

        term1Num++;

    }

    int* factors = fairFactors(term1Den);

    int term2Num = randInt(NUMMIN, NUMMAX);
    int term2Den = randIntArray(factors, factorsSize);

    while (gcd(term2Num, term2Den) > 1) {

        term2Num++;

    }

    if (term1Den == 1 && term2Den == 1) {

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

    if (denominator == 1) {

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

    int problemType = randIntArray(ptypeProblems, ptypeSize);

    switch (problemType) {

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

/*  Load problem types and default settings  */
    for (int i = 0; i < PTYPEMEM; i++) {

        ptypeProblems[i] = i;

    }

    for (int i = 0; i < PTYPEMEM; i++) {

        ptypeState[i] = 1;

    }

/*  Create AppVar if it doesn't exist  */
    var = ti_Open(appvar, "r");

    if(var == 0) {

        var = ti_Open(appvar, "w");

        if (ti_Write(&ptypeState, sizeof(int), PTYPEMEM, var) != PTYPEMEM) {

            os_PutStrFull("Failed initial write");
            while (!os_GetCSC());
            return 1;

        }

    }

/*  Read from AppVar  */
    var = ti_Open(appvar, "r");

    if (ti_Read(&ptypeState, sizeof(int), PTYPEMEM, var) != PTYPEMEM) {

        os_PutStrFull("Failed readback");
        while (!os_GetCSC());
        return 1;

    }

/*  Initialize graphics drawing  */
    gfx_Begin();

    gfx_SetTextTransparentColor(0xDF);

/*  Home page  */
    gfx_FillScreen(0xFF);

    redrawConfigMenu();

    configMenu();

/*  Update settings  */
    var = ti_Open(appvar, "w");

    if (ti_Write(&ptypeState, sizeof(int), PTYPEMEM, var) != PTYPEMEM || ptypeAllOff()) {

        ti_Delete(appvar);

        forceQuit(1);

    }

/*  Available problem types  */
    for (int i = 0; i < ptypeSize; i++) {

        if (ptypeState[i] == 0) {

            ptypeRemoveItem(i);

            i--;

        }

    }

/*  Draw screen layout  */
    gfx_FillScreen(0xFF);
    gfx_SetTextScale(1, 1);
    
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

        } else if (score > 0) {

            score--;

        }

        clearGuess();

    } while(difference > 0);

/*  Reset screen, final display  */
    gfx_FillScreen(0xFF);
    printCentered("Program completed", 80);

    printCentered(scoreDisp, 120);

/*  Ensure that the slot is closed  */
    ti_Close(var);
    var = 0;

/*  Waits for a key  */
    while (!os_GetCSC());

/*  End graphics drawing  */
    gfx_End();

    return 0;

}