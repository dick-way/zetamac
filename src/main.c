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

int factors[8];
int factorsSize = 0;

/*  exponentiation/root                */

const int BASEMIN = 2;
const int BASEMAX = 9;
const double BASEWEIGHTS[8] = {0.30, 0.16, 0.13, 0.13, 0.07, 0.07, 0.07, 0.07};

const int EXPMIN = 3;
const int EXPMAX[8] = {8, 4, 4, 4, 3, 3, 3, 3};

int ptypeProblems[PTYPEMEM];
double ptypeWeights[PTYPEMEM];  /*  TODO: Weighted probabilities, customization  */
int ptypeState[PTYPEMEM];

int ptypeSize = 9;
int ptypeSelection = 9;

const char* ptypeName[] = {"Addition", "Subtraction", "Multiplication", "Division", "Factorials", "Addition - Fraction", "Subtraction - Fraction", "Exponentiation", "Exponentiation - Inverse"};

const char* character[12] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "/", "-"};

kb_lkey_t inputMap[12] = {

        kb_Key0,
        kb_Key1,
        kb_Key2,
        kb_Key3,
        kb_Key4,
        kb_Key5,
        kb_Key6,
        kb_Key7,
        kb_Key8,
        kb_Key9,
        kb_KeyDiv,
        kb_KeyChs

};

kb_key_t key1, key3, key4, key5, key6, key7;
int inputLock[19];

char problem[17];
char answer[7];
char guess[7];

char scoreDisp[4];
int score = 0;
int scoreBonus = 0;

const int spacingX = 4;
const int spacingY = 4;

float timer_seconds = 90.0f;
float difference;

clock_t start;
char timerDisp[10];
int digitShift = 8;

uint8_t header = 0x4A;

uint8_t var;
const char* appvar = "Zetamac";

int problemDispType = 0;
char problemDispTerm[4][4];
char problemDispOperation[2];

int problemTerm[2];

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

void appendItem(int size, int item) {

    factors[size] = item;

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
    gfx_FillRectangle((GFX_LCD_WIDTH / 2) - 40, 65, 80, 40);

    printCentered(problem, 80);

    if (problemDispType == 1) {

        printCentered(problemDispOperation, 80);

        gfx_SetColor(0x00);
        gfx_FillRectangle((GFX_LCD_WIDTH / 2) - 22, 83, 12, (problemTerm[0] == 1) ? 0 : 2);
        gfx_FillRectangle((GFX_LCD_WIDTH / 2) + 8, 83, 12, (problemTerm[1] == 1) ? 0 : 2);

        int shift[4] = {0, 0, 0, 0};

        for (int i = 0; i < 4; i++) {

            if(strlen(problemDispTerm[i]) == 2) {

                shift[i] = 3;

            }

        }

        gfx_SetTextFGColor(0x00);
        gfx_SetTextBGColor(0xFF);

        gfx_PrintStringXY(problemDispTerm[0], (GFX_LCD_WIDTH / 2) - 20 - shift[0], (problemTerm[0] == 1) ? 80 : 74);

        if(problemTerm[0] != 1) {  gfx_PrintStringXY(problemDispTerm[1], (GFX_LCD_WIDTH / 2) - 20 - shift[1], 86);  }

        gfx_PrintStringXY(problemDispTerm[2], (GFX_LCD_WIDTH / 2) + 10 - shift[2], (problemTerm[1] == 1) ? 80 : 74);
        
        if(problemTerm[1] != 1) {  gfx_PrintStringXY(problemDispTerm[3], (GFX_LCD_WIDTH / 2) + 10 - shift[3], 86);  }

    } else if (problemDispType == 2) {

        printCentered(problemDispTerm[0], 80);

        gfx_SetTextFGColor(0x00);
        gfx_SetTextBGColor(0xFF);

        gfx_PrintStringXY(problemDispTerm[1], (GFX_LCD_WIDTH / 2) + 5, 76);

    } else if (problemDispType == 3) {

        printCentered(problemDispTerm[0], 80);

        gfx_SetTextFGColor(0x00);
        gfx_SetTextBGColor(0xFF);

        gfx_PrintStringXY(problemDispTerm[1], ((GFX_LCD_WIDTH - gfx_GetStringWidth(problemDispTerm[0])) / 2) - 15, 76);

        gfx_SetColor(0x00);

        gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth(problemDispTerm[0])) / 2) - 5, 74, 2, 14);
        gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth(problemDispTerm[0])) / 2) - 5, 74, gfx_GetStringWidth(problemDispTerm[0]) + 7, 2);

        gfx_Line(((GFX_LCD_WIDTH - gfx_GetStringWidth(problemDispTerm[0])) / 2) - 8, 85, ((GFX_LCD_WIDTH - gfx_GetStringWidth(problemDispTerm[0])) / 2) - 5, 88);

    }

    snprintf(scoreDisp, sizeof(scoreDisp), "%i", score);

    gfx_SetColor(header);
    gfx_FillRectangle(290, 0, 30, 20);

    gfx_SetTextFGColor(0xFF);
    gfx_SetTextBGColor(header);
    gfx_PrintStringXY(scoreDisp, GFX_LCD_WIDTH - 6 - (gfx_GetStringWidth(scoreDisp)), 6);

    gfx_SetTextFGColor(0x00);
    gfx_SetTextBGColor(0xFF);
    gfx_PrintStringXY("V 1.03", 6, GFX_LCD_HEIGHT - 14);

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

void textEntry() {  /*  TODO: line feed  */

    do {

        kb_Scan();

        key1 = kb_Data[1];
        key6 = kb_Data[6];

        if (key1 & kb_Mode) {

            forceQuit(0);

        }
        
        for (int i = 0; i < 12; i++) {

            if (kb_Data[inputMap[i] >> 8] & inputMap[i] && inputLock[i] == 0) {

                appendGuess(character[i]);

                inputLock[i] = 1;

            } else if (!(kb_Data[inputMap[i] >> 8] & inputMap[i])) {

                inputLock[i] = 0;

            }

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
        ptypeWeights[i] = ptypeWeights[i + 1];
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

void fairFactors(int term1Den) {

    factorsSize = 0;

    if (1 < term1Den && term1Den < 6 && randInt(0, 1) == 1) {

        for (int i = 2; i < 6; i++) {

            if (term1Den != i && (i % term1Den != 0 || term1Den % i != 0)) {

                appendItem(factorsSize, i);
                factorsSize++;

            }

        }

        scoreBonus = 2;

    } else {

        for (int i = DENMIN; i < term1Den; i++) {

            if (term1Den % i == 0) {

                appendItem(factorsSize, i);
                factorsSize++;

            }

        }

        for (int i = term1Den + 1; i < DENMAX; i++) {

            if (i % term1Den == 0) {

                appendItem(factorsSize, i);
                factorsSize++;

            }

        }

        scoreBonus = 1;
    
    }

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

    fairFactors(term1Den);

    int term2Num = randInt(NUMMIN, NUMMAX);
    int term2Den = randIntArray(factors, factorsSize);

    while (gcd(term2Num, term2Den) > 1) {

        term2Num++;

    }

    problem[0] = '\0';

    problemDispType = 1;

    snprintf(problemDispTerm[0], 4, "%i", term1Num);
    snprintf(problemDispTerm[1], 4, "%i", term1Den);
    snprintf(problemDispTerm[2], 4, "%i", term2Num);
    snprintf(problemDispTerm[3], 4, "%i", term2Den);

    snprintf(problemDispOperation, 2, "%c", operation);

    problemTerm[0] = term1Den;
    problemTerm[1] = term2Den;

    int denominator = lcm(term1Den, term2Den);
    int scaled1Num = term1Num * (denominator / term1Den);
    int scaled2Num = term2Num * (denominator / term2Den);
    int numerator = (operation == ADD) ? scaled1Num + scaled2Num : scaled1Num - scaled2Num;

    int gcdivisor = gcd(abs(numerator), denominator);

    if(gcdivisor > 1) {

        scoreBonus++;

    }

    numerator /= gcdivisor;
    denominator /= gcdivisor;

    if (denominator == 1) {

        snprintf(answer, sizeof(answer), "%d", numerator);

    } else {
        
        snprintf(answer, sizeof(answer), "%d/%d", numerator, denominator);

    }

}

void exponentiation() {

    int term1 = randIntWeighted(BASEMIN, BASEMAX, BASEWEIGHTS);
    int term2 = randInt(3, EXPMAX[term1 - BASEMIN]);

    problem[0] = '\0';

    problemDispType = 2;

    snprintf(problemDispTerm[0], 4, "%i", term1);
    snprintf(problemDispTerm[1], 4, "%i", term2);

    snprintf(answer, sizeof(answer), "%i", powInt(term1, term2));

}

void exponentiationInverse() {

    int term1 = randIntWeighted(BASEMIN, BASEMAX, BASEWEIGHTS);
    int term2 = randInt(3, EXPMAX[term1 - BASEMIN]);

    problem[0] = '\0';

    problemDispType = 3;

    snprintf(problemDispTerm[0], 4, "%i", powInt(term1, term2));
    snprintf(problemDispTerm[1], 4, "%i", term2);

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

        ptypeWeights[i] = 0.11;

    }

    for (int i = 0; i < PTYPEMEM; i++) {

        ptypeState[i] = 1;

    }

/*  Create AppVar if it doesn't exist  */
    var = ti_Open(appvar, "r");

    if(var == 0) {

        var = ti_Open(appvar, "w");

        if (ti_Write(&ptypeState, sizeof(int), PTYPEMEM, var) != PTYPEMEM) {

            os_PutStrFull("Failed write");
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

            score += scoreBonus + 1;

        } else if (score > 0) {

            score--;

        }

        scoreBonus = 0;

        problemDispType = 0;

        clearGuess();

    } while(difference > 0);

/*  Reset screen, final display  */
    gfx_FillScreen(0xFF);
    printCentered("program completed", 80);

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