#include "mainwindow.h"
#include <QLineEdit>
#include <QTextEdit>
#include <cstring>
#include <iostream>
#include <QInputDialog>
#include <QRadioButton>

using namespace std;

//---------------------------------------------------------------------------
const  int AL    =  10;  /* LENGTH OF IDENTIFIERS */
const  int NORW  =  19;  /* # OF RESERVED WORDS */  // 关键字个数
const  int TXMAX = 100;  /* LENGTH OF IDENTIFIER TABLE */
const  int NMAX  =  19;  /* MAX NUMBER OF DEGITS IN NUMBERS */
const  int AMAX  =2047;  /* MAXIMUM ADDRESS */
const  int LEVMAX=   3;  /* MAX DEPTH OF BLOCK NESTING */
const  int CXMAX = 200;  /* SIZE OF CODE ARRAY */
const int SYMNUM = 46;  // SYM个数

typedef enum  { NUL, IDENT, NUMBER, PLUS, MINUS, TIMES,
               SLASH, ODDSYM, EQL, NEQ, LSS, LEQ, GTR, GEQ,
               LPAREN, RPAREN, COMMA, SEMICOLON, PERIOD,
               BECOMES, BEGINSYM, ENDSYM, IFSYM, THENSYM,
               WHILESYM, WRITESYM, READSYM, DOSYM, CALLSYM,
               CONSTSYM, VARSYM, PROCSYM, PROGSYM,
               ELSESYM,FORSYM,STEPSYM,UNTILSYM,RETURNSYM,ORSYM,
               PLUSBECOMES,MINUSBECOMES,NOTESYM,ANDSYM,TIMESBECOMES,SLASHBECOMES,
               INCRSYM, DECRSYM


} SYMBOL;


//typedef  int *SYMSET; // SET OF SYMBOL;
//typedef  char ALFA[11];
//typedef  enum { CONSTANT, VARIABLE, PROCEDUR } OBJECTS ;
//typedef  enum { LIT, OPR, LOD, STO, CAL, INI, JMP, JPC } FCT;
typedef struct {
    FCT F;     /*FUNCTION CODE*/
    int L; 	/*0..LEVMAX  LEVEL*/
    int A;     /*0..AMAX    DISPLACEMENT ADDR*/
} INSTRUCTION;
/* LIT O A -- LOAD CONSTANT A             */
/* OPR 0 A -- EXECUTE OPR A               */
/* LOD L A -- LOAD VARIABLE L,A           */
/* STO L A -- STORE VARIABLE L,A          */
/* CAL L A -- CALL PROCEDURE A AT LEVEL L */
/* INI 0 A -- INCREMET T-REGISTER BY A    */
/* JMP 0 A -- JUMP TO A                   */
/* JPC 0 A -- JUMP CONDITIONAL TO A       */
char   CH;  /*LAST CHAR READ*/
SYMBOL SYM; /*LAST SYMBOL READ*/
ALFA   ID;  /*LAST IDENTIFIER READ*/
int    NUM; /*LAST NUMBER READ*/
int    CC;  /*CHARACTER COUNT*/
int    LL;  /*LINE LENGTH*/
int    CX;  /*CODE ALLOCATION INDEX*/
char   LINE[81];
INSTRUCTION  CODE[CXMAX];
ALFA    KWORD[NORW+1];
SYMBOL  WSYM[NORW+1];
SYMBOL  SSYM['^'+1];
ALFA    MNEMONIC[9];
SYMSET  DECLBEGSYS, STATBEGSYS, FACBEGSYS;

struct {
    ALFA NAME;
    OBJECTS KIND;
    union {
        int VAL;   /*CONSTANT*/
        struct { int LEVEL,ADR,SIZE; } vp;  /*VARIABLE,PROCEDUR:*/
    };
} TABLE[TXMAX];

FILE *FIN,*FOUT;
int ERR;

void EXPRESSION(SYMSET FSYS, int LEV, int &TX);
void TERM(SYMSET FSYS, int LEV, int &TX);
//---------------------------------------------------------------------------
int SymIn(SYMBOL SYM, SYMSET S1) {
    return S1[SYM];
}
//---------------------------------------------------------------------------

SYMSET SymSetUnion(SYMSET S1, SYMSET S2) {
    SYMSET S=(SYMSET)malloc(sizeof(int)*SYMNUM);
    for (int i=0; i<SYMNUM; i++)
        if (S1[i] || S2[i]) S[i]=1;
        else S[i]=0;
    return S;
}
//---------------------------------------------------------------------------
SYMSET SymSetAdd(SYMBOL SY, SYMSET S) {
    SYMSET S1;
    S1=(SYMSET)malloc(sizeof(int)*SYMNUM);
    for (int i=0; i<SYMNUM; i++) S1[i]=S[i];
    S1[SY]=1;
    return S1;
}
//---------------------------------------------------------------------------
SYMSET SymSetNew(SYMBOL a) {
    SYMSET S; int i,k;
    S=(SYMSET)malloc(sizeof(int)*SYMNUM);
    for (i=0; i<SYMNUM; i++) S[i]=0;
    S[a]=1;
    return S;
}
//---------------------------------------------------------------------------
SYMSET SymSetNew(SYMBOL a, SYMBOL b) {
    SYMSET S; int i,k;
    S=(SYMSET)malloc(sizeof(int)*SYMNUM);
    for (i=0; i<SYMNUM; i++) S[i]=0;
    S[a]=1;  S[b]=1;
    return S;
}
//---------------------------------------------------------------------------
SYMSET SymSetNew(SYMBOL a, SYMBOL b, SYMBOL c) {
    SYMSET S; int i,k;
    S=(SYMSET)malloc(sizeof(int)*SYMNUM);
    for (i=0; i<SYMNUM; i++) S[i]=0;
    S[a]=1;  S[b]=1; S[c]=1;
    return S;
}
//---------------------------------------------------------------------------
SYMSET SymSetNew(SYMBOL a, SYMBOL b, SYMBOL c, SYMBOL d) {
    SYMSET S; int i,k;
    S=(SYMSET)malloc(sizeof(int)*SYMNUM);
    for (i=0; i<SYMNUM; i++) S[i]=0;
    S[a]=1;  S[b]=1; S[c]=1; S[d]=1;
    return S;
}
//---------------------------------------------------------------------------
SYMSET SymSetNew(SYMBOL a, SYMBOL b, SYMBOL c, SYMBOL d,SYMBOL e) {
    SYMSET S; int i,k;
    S=(SYMSET)malloc(sizeof(int)*SYMNUM);
    for (i=0; i<SYMNUM; i++) S[i]=0;
    S[a]=1;  S[b]=1; S[c]=1; S[d]=1; S[e]=1;
    return S;
}
//---------------------------------------------------------------------------
SYMSET SymSetNew(SYMBOL a, SYMBOL b, SYMBOL c, SYMBOL d,SYMBOL e, SYMBOL f) {
    SYMSET S; int i,k;
    S=(SYMSET)malloc(sizeof(int)*SYMNUM);
    for (i=0; i<SYMNUM; i++) S[i]=0;
    S[a]=1;  S[b]=1; S[c]=1; S[d]=1; S[e]=1; S[f]=1;
    return S;
}
//---------------------------------------------------------------------------
SYMSET SymSetNULL() {
    SYMSET S; int i,n,k;
    S=(SYMSET)malloc(sizeof(int)*SYMNUM);
    for (i=0; i<SYMNUM; i++) S[i]=0;
    return S;
}
//---------------------------------------------------------------------------
void MainWindow::Error(int n) {
    QString qs = QString("***");
    for(int i=0;i<CC-1;i++){
        qs.append(" ");
    }
    qs.append("^");
    string s = qs.toStdString();
    // BCB API:static AnsiString StringOfChar(char ch, int count)  返回一个包含count个字符ch的字符串。
    //string s = "***"+AnsiString::StringOfChar(' ', CC-1)+"^";
    //  Form1->printls(s.c_str(),n);   fprintf(FOUT,"%s%d\n", s.c_str(), n);
    printls(s.c_str(),n);   fprintf(FOUT,"%s%d\n", s.c_str(), n);
    ERR++;
} /*Error*/
//---------------------------------------------------------------------------
void MainWindow::GetCh() {
    if (CC==LL) {
        if (feof(FIN)) {
            //Form1->printfs("PROGRAM INCOMPLETE");
            printfs("PROGRAM INCOMPLETE");
            fprintf(FOUT,"PROGRAM INCOMPLETE\n");
            fclose(FOUT);
            exit(0);
        }
        LL=0; CC=0;
        CH=' ';
        while (!feof(FIN) && CH!=10)
        { CH=fgetc(FIN);  LINE[LL++]=CH; }
        LINE[LL-1]=' ';  LINE[LL]=0;
        string s=to_string(CX);
        while(s.length()<3) s=" "+s;
        s=s+" "+LINE;
        //    Form1->printfs(s.c_str());
        printfs(s.c_str());
        fprintf(FOUT,"%s\n",s.c_str());
    }
    CH=LINE[CC++];
} /*GetCh()*/
//---------------------------------------------------------------------------
void MainWindow::GetSym() {
    int i, J, K;   // 用于二分查找和临时计数
    ALFA A;        // 字符数组，存储当前读取的标识符（ALFA 通常是 char[AL] 类型）

    // 1. 跳过空白字符（空格、换行等）
    while (CH <= ' ') GetCh();

    // 2. 处理标识符或关键字（以字母开头）
    if (CH >= 'A' && CH <= 'Z') {
        K = 0;
        do {
            if (K < AL) A[K++] = CH; // 将字符存入临时数组 A
            GetCh();
        } while ((CH >= 'A' && CH <= 'Z') || (CH >= '0' && CH <= '9')); // 支持字母数字组合
        A[K] = '\0'; // 添加字符串结束符

        strcpy(ID, A); // 复制到全局变量 ID
        i = 1; J = NORW; // NORW 是关键字表大小
        // 二分查找关键字表（KWORD 是关键字字符串数组）
        do {
            K = (i + J) / 2;
            if (strcmp(ID, KWORD[K]) <= 0) J = K - 1;
            if (strcmp(ID, KWORD[K]) >= 0) i = K + 1;
        } while (i <= J);

        // 若命中关键字，SYM 设为对应符号（如 IFSYM），否则为标识符
        if (i - 1 > J) SYM = WSYM[K]; // WSYM 存储关键字对应的符号码
        else SYM = IDENT; // 标识符
    }

    // 3. 处理数字字面量
    else if (CH >= '0' && CH <= '9') {
        K = 0; NUM = 0; SYM = NUMBER;
        do {
            NUM = 10 * NUM + (CH - '0'); // 转换为整数
            K++; GetCh();
        } while (CH >= '0' && CH <= '9');
        if (K > NMAX) Error(30); // 数字长度超限报错
    }

    // 4. 处理运算符和边界符
    else {
        switch (CH) {
        case ':':
            GetCh();
            if (CH == '=') { SYM = BECOMES; GetCh(); } // ":=" 赋值符号
            else SYM = NUL; // 非法或未定义
            break;

        case '<': // 处理 <, <=, <>
            GetCh();
            if (CH == '=') { SYM = LEQ; GetCh(); }     // "<=" 小于等于
            else if (CH == '>') { SYM = NEQ; GetCh(); } // "<>" 不等于（部分方言）
            else SYM = LSS; // "<" 小于
            break;

        case '>': // 处理 >, >=
            GetCh();
            if (CH == '=') { SYM = GEQ; GetCh(); } // ">=" 大于等于
            else SYM = GTR; // ">" 大于
            break;

            // 5. 扩展运算符
        case '+':
            GetCh();
            if (CH == '=') { SYM = PLUSBECOMES; GetCh(); }
            else if (CH == '+') { SYM = INCRSYM; GetCh(); }  // 识别自增
            else SYM = PLUS;
            break;

        case '-':
            GetCh();
            if (CH == '=') { SYM = MINUSBECOMES; GetCh(); }
            else if (CH == '-') { SYM = DECRSYM; GetCh(); }  // 识别自减
            else SYM = MINUS;
            break;

        case '/':
            GetCh();
            if (CH == '=') { SYM = SLASHBECOMES; GetCh(); } // "/=" 复合除法赋值
            else SYM = SLASH; // "/" 除法
            break;

        case '*':
            GetCh();
            if (CH == '=') { SYM = TIMESBECOMES; GetCh(); } // "*=" 复合乘法赋值
            else SYM = TIMES; // "*" 乘法
            break;

        case '&':
            GetCh();
            SYM = ANDSYM; // "&" 逻辑与
            break;

        case '|':
            GetCh();
            SYM = ORSYM; // "|" 逻辑或（需连续两个 '|' 的实现在其他版本）
            break;

        case '!':
            GetCh();
            SYM = NOTESYM; // "!" 逻辑非
            break;

        default: // 其他单字符运算符（如 +, -, (, ) 等）
            SYM = SSYM[CH]; // SSYM 是单字符符号映射表（如 '+' -> PLUS）
            GetCh();
        }
    }
} // GetSym()
//---------------------------------------------------------------------------
void MainWindow::GEN(FCT X, int Y, int Z) {
    if (CX>CXMAX) {
        //    Form1->printfs("PROGRAM TOO LONG");
        printfs("PROGRAM TOO LONG");
        fprintf(FOUT,"PROGRAM TOO LONG\n");
        fclose(FOUT);
        exit(0);
    }
    CODE[CX].F=X; CODE[CX].L=Y; CODE[CX].A=Z;
    CX++;
} /*GEN*/
//---------------------------------------------------------------------------
void MainWindow::TEST(SYMSET S1, SYMSET S2, int N) {
    if (!SymIn(SYM,S1)) {
        Error(N);
        while (!SymIn(SYM,SymSetUnion(S1,S2))) GetSym();
    }
} /*TEST*/
//---------------------------------------------------------------------------
void MainWindow::ENTER(OBJECTS K, int LEV, int &TX, int &DX) { /*ENTER OBJECT INTO TABLE*/
    TX++;
    strcpy(TABLE[TX].NAME,ID); TABLE[TX].KIND=K;
    switch (K) {
    case CONSTANT:
        if (NUM>AMAX) { Error(31); NUM=0; }
        TABLE[TX].VAL=NUM;
        break;
    case VARIABLE:
        TABLE[TX].vp.LEVEL=LEV; TABLE[TX].vp.ADR=DX; DX++;
        break;
    case PROCEDUR:
        TABLE[TX].vp.LEVEL=LEV;
        break;
    }
} /*ENTER*/
//---------------------------------------------------------------------------
int MainWindow::POSITION(ALFA ID, int TX) { /*FIND IDENTIFIER IN TABLE*/
    int i=TX;
    strcpy(TABLE[0].NAME,ID);
    while (strcmp(TABLE[i].NAME,ID)!=0) i--;
    return i;
} /*POSITION*/
//---------------------------------------------------------------------------
void MainWindow::ConstDeclaration(int LEV,int &TX,int &DX) {
    if (SYM==IDENT) {
        GetSym();
        if (SYM==EQL||SYM==BECOMES) {
            if (SYM==BECOMES) Error(1);
            GetSym();
            if (SYM==NUMBER) { ENTER(CONSTANT,LEV,TX,DX); GetSym(); }
            else Error(2);
        }
        else Error(3);
    }
    else Error(4);
} /*ConstDeclaration()*/
//---------------------------------------------------------------------------
void MainWindow::VarDeclaration(int LEV,int &TX,int &DX) {
    if (SYM==IDENT) { ENTER(VARIABLE,LEV,TX,DX); GetSym(); }
    else Error(4);
} /*VarDeclaration()*/
//---------------------------------------------------------------------------
void MainWindow::ListCode(int CX0) {  /*LIST CODE GENERATED FOR THIS Block*/
    if(visibleRadioButton->isChecked())
        for (int i=CX0; i<CX; i++) {
            string s=to_string(i);
            while(s.length()<3)s=" "+s;
            s=s+" "+MNEMONIC[CODE[i].F]+" "+to_string(CODE[i].L)+" "+to_string(CODE[i].A);
            //      Form1->printfs(s.c_str());
            printfs(s.c_str());
            //      fprintf(FOUT,"%3d%5s%4d%4d\n",i,MNEMONIC[CODE[i].F],CODE[i].L,CODE[i].A);
            fprintf(FOUT,"%s\n",s.c_str());
        }
} /*ListCode()*/
//---------------------------------------------------------------------------
void MainWindow::FACTOR(SYMSET FSYS, int LEV, int &TX) {
    int i;
    TEST(FACBEGSYS,FSYS,24);
    while (SymIn(SYM,FACBEGSYS)) {

        if (SYM == INCRSYM || SYM == DECRSYM) {
            SYMBOL op = SYM;  // 保存操作符
            GetSym();  // 读取下一个符号
            if (SYM != IDENT) {
                Error(11);  // 期望标识符
                break;
            }

            i = POSITION(ID, TX);
            if (i == 0) Error(11);
            else if (TABLE[i].KIND != VARIABLE) {
                Error(12);  // 非变量不能自增自减
                i = 0;
            }

            if (i != 0) {
                // 前缀自增/自减：修改变量然后使用
                GEN(LOD, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);  // 加载变量
                GEN(LIT, 0, 1);  // 加载常数1
                GEN(OPR, 0, (op == INCRSYM) ? 2 : 3);  // 加法或减法
                GEN(STO, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);  // 存储回变量
                GEN(LOD, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);  // 重新加载新值
            }
            GetSym();  // 读取下一个符号
        }
        // 处理后缀自增自减
        else if (SYM == IDENT) {
            i = POSITION(ID, TX);
            if (i == 0) Error(11);
            else
                switch (TABLE[i].KIND) {
                case CONSTANT:
                    GEN(LIT,0,TABLE[i].VAL);
                    break;
                case VARIABLE:
                    GEN(LOD,LEV-TABLE[i].vp.LEVEL,TABLE[i].vp.ADR);
                    break;
                case PROCEDUR:
                    Error(21);
                    break;
                }
            GetSym();

            // 检查是否有后缀运算符
            if (SYM == INCRSYM || SYM == DECRSYM) {
                if (i == 0 || TABLE[i].KIND != VARIABLE) {
                    Error(12);  // 非变量不能自增自减
                } else {
                    // 后缀自增/自减：先使用变量然后修改
                    GEN(LOD, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);  // 再次加载变量
                    GEN(LIT, 0, 1);  // 加载常数1
                    GEN(OPR, 0, (SYM == INCRSYM) ? 2 : 3);  // 加法或减法
                    GEN(STO, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);  // 存储回变量
                }
                GetSym();  // 读取下一个符号
            }
        }
        else if (SYM==NUMBER) {
            if (NUM>AMAX) { Error(31); NUM=0; }
            GEN(LIT,0,NUM); GetSym();
        }
        else if (SYM==LPAREN) {
            GetSym(); EXPRESSION(SymSetAdd(RPAREN,FSYS),LEV,TX);
            if (SYM==RPAREN) GetSym();
            else Error(22);
        }
        TEST(FSYS,FACBEGSYS,23);
    }
}/*FACTOR*/
//---------------------------------------------------------------------------
void MainWindow::TERM(SYMSET FSYS, int LEV, int &TX) {  /*TERM*/
    SYMBOL MULOP;
    FACTOR(SymSetUnion(FSYS,SymSetNew(TIMES,SLASH)), LEV,TX);
    while (SYM==TIMES || SYM==SLASH) {
        MULOP=SYM;  GetSym();
        FACTOR(SymSetUnion(FSYS,SymSetNew(TIMES,SLASH)),LEV,TX);
        if (MULOP==TIMES) GEN(OPR,0,4);
        else GEN(OPR,0,5);
    }
} /*TERM*/
//---------------------------------------------------------------------------
void MainWindow::EXPRESSION(SYMSET FSYS, int LEV, int &TX) {
    SYMBOL ADDOP;
    if (SYM==PLUS || SYM==MINUS) {
        ADDOP=SYM; GetSym();
        TERM(SymSetUnion(FSYS,SymSetNew(PLUS,MINUS)),LEV,TX);
        if (ADDOP==MINUS) GEN(OPR,0,1);
    }
    else TERM(SymSetUnion(FSYS,SymSetNew(PLUS,MINUS)),LEV,TX);
    while (SYM==PLUS || SYM==MINUS) {
        ADDOP=SYM; GetSym();
        TERM(SymSetUnion(FSYS,SymSetNew(PLUS,MINUS)),LEV,TX);
        if (ADDOP==PLUS) GEN(OPR,0,2);
        else GEN(OPR,0,3);
    }
} /*EXPRESSION*/
//---------------------------------------------------------------------------
void MainWindow::CONDITION(SYMSET FSYS,int LEV,int &TX) {
    SYMBOL RELOP;
    if (SYM==ODDSYM) { GetSym(); EXPRESSION(FSYS,LEV,TX); GEN(OPR,0,6); }
    else {
        EXPRESSION(SymSetUnion(SymSetNew(EQL,NEQ,LSS,LEQ,GTR,GEQ),FSYS),LEV,TX);
        if (!SymIn(SYM,SymSetNew(EQL,NEQ,LSS,LEQ,GTR,GEQ))) Error(20);
        else {
            RELOP=SYM; GetSym(); EXPRESSION(FSYS,LEV,TX);
            switch (RELOP) {
            case EQL: GEN(OPR,0,8);  break;
            case NEQ: GEN(OPR,0,9);  break;
            case LSS: GEN(OPR,0,10); break;
            case GEQ: GEN(OPR,0,11); break;
            case GTR: GEN(OPR,0,12); break;
            case LEQ: GEN(OPR,0,13); break;

            }
        }
    }
} /*CONDITION*/
//---------------------------------------------------------------------------
void MainWindow::STATEMENT(SYMSET FSYS,int LEV,int &TX) {   /*STATEMENT*/
    int i,CX1,CX2,CX3;
    switch (SYM) {
    case IDENT: {
        i = POSITION(ID, TX);
        if (i == 0) Error(11);
        else if (TABLE[i].KIND != VARIABLE) {
            Error(12);
            i = 0;
        }
        GetSym();
        if (SYM == BECOMES || SYM == PLUSBECOMES || SYM == MINUSBECOMES ||
            SYM == TIMESBECOMES || SYM == SLASHBECOMES) {

            SYMBOL op = SYM;
            GetSym();
            EXPRESSION(FSYS, LEV, TX);

            if (i != 0) {
                // 复合赋值需要先生成加载变量指令
                if (op != BECOMES) {
                    GEN(LOD, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);
                    switch (op) {
                    case PLUSBECOMES: GEN(OPR, 0, 2); break;  // 加法
                    case MINUSBECOMES: GEN(OPR, 0, 3); break; // 减法
                    case TIMESBECOMES: GEN(OPR, 0, 4); break; // 乘法
                    case SLASHBECOMES: GEN(OPR, 0, 5); break; // 除法
                    }
                }
                GEN(STO, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);
            }
        } else {
            Error(13);
        }
        break;
    }
    case READSYM:
        GetSym();
        if (SYM!=LPAREN) Error(34);
        else
            do {
                GetSym();
                if (SYM==IDENT) i=POSITION(ID,TX);
                else i=0;
                if (i==0) Error(35);
                else {
                    GEN(OPR,0,16);
                    GEN(STO,LEV-TABLE[i].vp.LEVEL,TABLE[i].vp.ADR);
                }
                GetSym();
            }while(SYM==COMMA);
        if (SYM!=RPAREN) {
            Error(33);
            while (!SymIn(SYM,FSYS)) GetSym();
        }
        else GetSym();
        break; /* READSYM */
    case WRITESYM:
        GetSym();
        if (SYM==LPAREN) {
            do {
                GetSym();
                EXPRESSION(SymSetUnion(SymSetNew(RPAREN,COMMA),FSYS),LEV,TX);
                GEN(OPR,0,14);
            }while(SYM==COMMA);
            if (SYM!=RPAREN) Error(33);
            else GetSym();
        }
        GEN(OPR,0,15);
        break; /*WRITESYM*/
    case CALLSYM:
        GetSym();
        if (SYM!=IDENT) Error(14);
        else {
            i=POSITION(ID,TX);
            if (i==0) Error(11);
            else
                if (TABLE[i].KIND==PROCEDUR)
                    GEN(CAL,LEV-TABLE[i].vp.LEVEL,TABLE[i].vp.ADR);
                else Error(15);
            GetSym();
        }
        break;
    case IFSYM:  // 当当前符号是IF关键字时
        GetSym();  // 获取下一个符号（跳过IF）
        // 解析条件表达式。传入的条件是：THENSYM（THEN）或DOSYM（DO）和当前作用域的符号集合（FSYS）
        CONDITION(SymSetUnion(SymSetNew(THENSYM,DOSYM),FSYS),LEV,TX);

        // 检查当前符号是否为THENSYM（THEN关键字）
        if (SYM==THENSYM)
            GetSym();  // 如果是，获取下一个符号（跳过THEN）
        else
            Error(16);  // 否则报错（缺少THEN）

        // 记录当前代码生成位置（CX1），用于后续回填跳转地址
        CX1=CX;
        // 生成条件跳转指令：JPC（条件假跳转）。此时跳转地址暂填0（待回填）
        GEN(JPC,0,0);

        // 解析THEN分支的语句（STATEMENT）。允许后面跟随ELSESYM（ELSE）或当前作用域的结束符号
        STATEMENT(SymSetUnion(SymSetNew(ELSESYM),FSYS),LEV,TX);

        // 检查当前符号是否是ELSESYM（ELSE关键字）
        if (SYM!=ELSESYM)
            // 如果没有ELSE分支，则直接将之前条件跳转的目标地址设为当前代码位置（CX）
            // （当条件为假时，跳转到这里，即跳过THEN块）
            CODE[CX1].A=CX;
        else {
            // 存在ELSE分支
            GetSym();  // 获取下一个符号（跳过ELSE）
            CX2 = CX;  // 记录当前位置（用于生成跳过ELSE块的跳转指令）（CX是代码索引）
            // 生成无条件跳转指令（JMP），目标地址暂填0（待回填）。此跳转用于跳过ELSE块
            GEN(JMP, 0, 0);
            // 回填条件跳转（JPC）的目标地址：当前CX（即指向刚生成的JMP指令之后）
            // 条件为假时跳转到这里，开始执行ELSE分支
            CODE[CX1].A = CX;

            // 解析ELSE分支的语句
            STATEMENT(FSYS, LEV, TX);

            // 回填之前生成的JMP指令的目标地址：当前CX（即ELSE块结束后的位置）
            CODE[CX2].A = CX;
        }
        break;

    case BEGINSYM:
        GetSym();
        STATEMENT(SymSetUnion(SymSetNew(SEMICOLON,ENDSYM),FSYS),LEV,TX);
        while (SymIn(SYM, SymSetAdd(SEMICOLON,STATBEGSYS))) {
            if (SYM==SEMICOLON) GetSym();
            else Error(10);
            STATEMENT(SymSetUnion(SymSetNew(SEMICOLON,ENDSYM),FSYS),LEV,TX);
        }
        if (SYM==ENDSYM) GetSym();
        else Error(17);
        break;
    case WHILESYM:
        CX1=CX; GetSym(); CONDITION(SymSetAdd(DOSYM,FSYS),LEV,TX);
        CX2=CX; GEN(JPC,0,0);
        if (SYM==DOSYM) GetSym();
        else Error(18);
        STATEMENT(FSYS,LEV,TX);
        GEN(JMP,0,CX1);
        CODE[CX2].A=CX;
        break;
    case ELSESYM:
        GetSym();
        STATEMENT(FSYS,LEV,TX);
        break;
    case FORSYM: {
        GetSym();  // 跳过FOR关键字

        // 解析变量标识符
        if (SYM != IDENT) {
            Error(4);
            while (!SymIn(SYM, FSYS)) GetSym();
            break;
        }
        i = POSITION(ID, TX);
        if (i == 0) Error(11);
        else if (TABLE[i].KIND != VARIABLE) {
            Error(12);
            i = 0;
        }
        GetSym();

        // 解析赋值符号
        if (SYM != BECOMES) Error(13);
        else GetSym();

        // 解析初始值表达式
        EXPRESSION(FSYS, LEV, TX);
        if (i != 0) GEN(STO, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);

        // 解析STEP关键字
        if (SYM != STEPSYM) Error(38);  // 自定义错误码
        else GetSym();

        // 解析步长表达式
        EXPRESSION(FSYS, LEV, TX);
        CX1 = CX;  // 记录步长代码结束位置
        GEN(STO, LEV, TABLE[i].vp.ADR + 1);  // 使用变量后一位存储步长

        // 解析UNTIL关键字
        if (SYM != UNTILSYM) Error(39);  // 自定义错误码
        else GetSym();

        // 解析终止值表达式
        EXPRESSION(FSYS, LEV, TX);
        CX2 = CX;  // 记录终止值代码结束位置
        GEN(STO, LEV, TABLE[i].vp.ADR + 2);  // 使用变量后两位存储终止值

        // 生成循环入口标签
        int loopStart = CX;

        // 加载循环变量和终止值进行比较
        GEN(LOD, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);
        GEN(LOD, LEV, TABLE[i].vp.ADR + 2);
        GEN(OPR, 0, LEQ);  // 比较循环变量 <= 终止值? (反向比较)

        // 解析DO关键字
        if (SYM != DOSYM) Error(18);
        else GetSym();

        // 生成条件跳转
        CX3 = CX;
        GEN(JPC, 0, 0);

        // 解析循环体语句
        STATEMENT(FSYS, LEV, TX);

        // 步长操作：变量 += 步长
        GEN(LOD, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);
        GEN(LOD, LEV, TABLE[i].vp.ADR + 1);
        GEN(OPR, 0, 2);  // 加法
        GEN(STO, LEV-TABLE[i].vp.LEVEL, TABLE[i].vp.ADR);

        // 生成循环跳转
        GEN(JMP, 0, loopStart);

        // 回填条件跳转地址
        CODE[CX3].A = CX;
        break;
    }
    case STEPSYM:
        GetSym();
        break;
    case UNTILSYM:
        GetSym();
        break;
    case RETURNSYM:
        GetSym();
        break;
    case DOSYM:
        GetSym();
        break;
    case TIMESBECOMES:
        GetSym();
        break;
    case SLASHBECOMES:
        GetSym();
        break;
    case ANDSYM:
        GetSym();
        break;
    case ORSYM:
        GetSym();
        break;
    case NOTESYM:
        GetSym();
        break;
    }
    TEST(FSYS,SymSetNULL(),19);
} /*STATEMENT*/
//---------------------------------------------------------------------------
void MainWindow::Block(int LEV, int TX, SYMSET FSYS) {
    int DX=3;    /*DATA ALLOCATION INDEX*/
    int TX0=TX;  /*INITIAL TABLE INDEX*/
    int CX0=CX;  /*INITIAL CODE INDEX*/
    TABLE[TX].vp.ADR=CX; GEN(JMP,0,0);
    if (LEV>LEVMAX) Error(32);
    do {
        if (SYM==CONSTSYM) {
            GetSym();
            do {
                ConstDeclaration(LEV,TX,DX);
                while (SYM==COMMA) {
                    GetSym();  ConstDeclaration(LEV,TX,DX);
                }
                if (SYM==SEMICOLON) GetSym();
                else Error(5);
            }while(SYM==IDENT);
        }
        if (SYM==VARSYM) {
            GetSym();
            do {
                VarDeclaration(LEV,TX,DX);
                while (SYM==COMMA) { GetSym(); VarDeclaration(LEV,TX,DX); }
                if (SYM==SEMICOLON) GetSym();
                else Error(5);
            }while(SYM==IDENT);
        }
        while ( SYM==PROCSYM) {
            GetSym();
            if (SYM==IDENT) { ENTER(PROCEDUR,LEV,TX,DX); GetSym(); }
            else Error(4);
            if (SYM==SEMICOLON) GetSym();
            else Error(5);
            Block(LEV+1,TX,SymSetAdd(SEMICOLON,FSYS));
            if (SYM==SEMICOLON) {
                GetSym();
                TEST(SymSetUnion(SymSetNew(IDENT,PROCSYM),STATBEGSYS),FSYS,6);
            }
            else Error(5);
        }
        TEST(SymSetAdd(IDENT,STATBEGSYS), DECLBEGSYS,7);
    }while(SymIn(SYM,DECLBEGSYS));
    CODE[TABLE[TX0].vp.ADR].A=CX;
    TABLE[TX0].vp.ADR=CX;   /*START ADDR OF CODE*/
    TABLE[TX0].vp.SIZE=DX;  /*SIZE OF DATA SEGMENT*/
    GEN(INI,0,DX);
    STATEMENT(SymSetUnion(SymSetNew(SEMICOLON,ENDSYM),FSYS),LEV,TX);
    GEN(OPR,0,0);  /*RETURN*/
    TEST(FSYS,SymSetNULL(),8);
    ListCode(CX0);
} /*Block*/
//---------------------------------------------------------------------------
int MainWindow::BASE(int L,int B,int S[]) {
    int B1=B; /*FIND BASE L LEVELS DOWN*/
    while (L>0) { B1=S[B1]; L=L-1; }
    return B1;
} /*BASE*/
//---------------------------------------------------------------------------
void MainWindow::Interpret() {
    const int STACKSIZE = 500;
    int P,B,T; 		/*PROGRAM BASE TOPSTACK REGISTERS*/
    INSTRUCTION I;
    int S[STACKSIZE];  	/*DATASTORE*/
    //  Form1->printfs("~~~ RUN PL0 ~~~");
    printfs("~~~ RUN PL0 ~~~");
    fprintf(FOUT,"~~~ RUN PL0 ~~~\n");
    T=0; B=1; P=0;
    S[1]=0; S[2]=0; S[3]=0;
    do {
        I=CODE[P]; P=P+1;
        switch (I.F) {
        case LIT: T++; S[T]=I.A; break;
        case OPR:
            switch (I.A) { /*OPERATOR*/
            case 0: /*RETURN*/ T=B-1; P=S[T+3]; B=S[T+2]; break;
            case 1: S[T]=-S[T];  break;
            case 2: T--; S[T]=S[T]+S[T+1];   break;
            case 3: T--; S[T]=S[T]-S[T+1];   break;
            case 4: T--; S[T]=S[T]*S[T+1];   break;
            case 5: T--; S[T]=S[T]/S[T+1]; break;
            case 6: S[T]=(S[T]%2!=0);        break;
            case 8: T--; S[T]=S[T]==S[T+1];  break;
            case 9: T--; S[T]=S[T]!=S[T+1];  break;
            case 10: T--; S[T]=S[T]<S[T+1];   break;
            case 11: T--; S[T]=S[T]>=S[T+1];  break;
            case 12: T--; S[T]=S[T]>S[T+1];   break;
            case 13: T--; S[T]=S[T]<=S[T+1];  break;
            case 14: printls("",S[T]); fprintf(FOUT,"%d\n",S[T]); T--;
                break;
            case 15: /*Form1->printfs(""); fprintf(FOUT,"\n"); */ break;
            case 16: T++;  //S[T]=InputBox("输入","请键盘输入：", 0).ToInt();
                bool ok;
                int in=QInputDialog::getInt(this,tr(" 输入"),tr("请键盘输入："),0,0,200,2,&ok);
                if(ok){
                    S[T]=in;
                }else{
                    S[T]=0;
                }
                printls("? ",S[T]); fprintf(FOUT,"? %d\n",S[T]);
                break;
            }
            break;
        case LOD: T++; S[T]=S[BASE(I.L,B,S)+I.A]; break;
        case STO: S[BASE(I.L,B,S)+I.A]=S[T]; T--; break;
        case CAL: /*GENERAT NEW Block MARK*/
            S[T+1]=BASE(I.L,B,S); S[T+2]=B; S[T+3]=P;
            B=T+1; P=I.A; break;
        case INI: T=T+I.A;  break;
        case JMP: P=I.A; break;
        case JPC: if (S[T]==0) P=I.A;  T--;  break;
        } /*switch*/
    }while(P!=0);
    //  Form1->printfs("~~~ END PL0 ~~~");
    printfs("~~~ END PL0 ~~~");
    fprintf(FOUT,"~~~ END PL0 ~~~\n");
} /*Interpret*/
//---------------------------------------------------------------------------

void MainWindow::runClicked(){
    for (CH=' '; CH<='^'; CH++) SSYM[CH]=NUL;
    strcpy(KWORD[1],"BEGIN");     strcpy(KWORD[2],"CALL");
    strcpy(KWORD[3],"CONST");     strcpy(KWORD[4],"DO");
    strcpy(KWORD[5],"ELSE");      strcpy(KWORD[6],"END");
    strcpy(KWORD[7],"FOR");       strcpy(KWORD[8],"IF");
    strcpy(KWORD[9],"ODD");       strcpy(KWORD[10],"PROCEDURE");
    strcpy(KWORD[11],"PROGRAM");  strcpy(KWORD[12],"READ");
    strcpy(KWORD[13],"RETURN");   strcpy(KWORD[14],"STEP");
    strcpy(KWORD[15],"THEN");     strcpy(KWORD[16],"UNTIL");
    strcpy(KWORD[17],"VAR");      strcpy(KWORD[18],"WHILE");
    strcpy(KWORD[19],"WRITE");

    WSYM[1]=BEGINSYM;   WSYM[2]=CALLSYM;
    WSYM[3]=CONSTSYM;   WSYM[4]=DOSYM;
    WSYM[5]=ELSESYM;     WSYM[6]=ENDSYM;
    WSYM[7]=FORSYM;     WSYM[8]=IFSYM;
    WSYM[9]=ODDSYM;     WSYM[10]=PROCSYM;
    WSYM[11]=PROGSYM;    WSYM[12]=READSYM;
    WSYM[13]=RETURNSYM;   WSYM[14]=STEPSYM;
    WSYM[15]=THENSYM;    WSYM[16]=UNTILSYM;
    WSYM[17]=VARSYM;    WSYM[18]=WHILESYM;
    WSYM[19]=WRITESYM;

    SSYM['+']=PLUS;      SSYM['-']=MINUS;
    SSYM['*']=TIMES;     SSYM['/']=SLASH;
    SSYM['(']=LPAREN;    SSYM[')']=RPAREN;
    SSYM['=']=EQL;       SSYM[',']=COMMA;
    SSYM['.']=PERIOD;    //SSYM['#']=NEQ;
    SSYM[';']=SEMICOLON; SSYM['&']=ANDSYM;
    SSYM['|']=ORSYM;     SSYM['!']=NOTESYM;
    strcpy(MNEMONIC[LIT],"LIT");   strcpy(MNEMONIC[OPR],"OPR");
    strcpy(MNEMONIC[LOD],"LOD");   strcpy(MNEMONIC[STO],"STO");
    strcpy(MNEMONIC[CAL],"CAL");   strcpy(MNEMONIC[INI],"INI");
    strcpy(MNEMONIC[JMP],"JMP");   strcpy(MNEMONIC[JPC],"JPC");

    DECLBEGSYS=(int*)malloc(sizeof(int)*SYMNUM);
    STATBEGSYS=(int*)malloc(sizeof(int)*SYMNUM);
    FACBEGSYS =(int*)malloc(sizeof(int)*SYMNUM);
    for(int j=0; j<SYMNUM; j++) {
        DECLBEGSYS[j]=0;  STATBEGSYS[j]=0;  FACBEGSYS[j] =0;
    }
    DECLBEGSYS[CONSTSYM]=1;
    DECLBEGSYS[VARSYM]=1;
    DECLBEGSYS[PROCSYM]=1;
    STATBEGSYS[BEGINSYM]=1;
    STATBEGSYS[CALLSYM]=1;
    STATBEGSYS[ELSESYM]=1;
    STATBEGSYS[IFSYM]=1;
    STATBEGSYS[FORSYM]=1;
    STATBEGSYS[WHILESYM]=1;
    STATBEGSYS[WRITESYM]=1;
    FACBEGSYS[IDENT] =1;
    FACBEGSYS[NUMBER]=1;
    FACBEGSYS[LPAREN]=1;
    FACBEGSYS[INCRSYM]=1;  // 添加自增作为因子开始
    FACBEGSYS[DECRSYM]=1;  // 添加自减作为因子开始

    const string filename = sourceLineEdit->text().toStdString();
    const string pl0name = filename+".PL0";
    const string codname = filename+".COD";
    if ((FIN=fopen(pl0name.c_str(),"r"))!=0) {
        FOUT=fopen(codname.c_str(),"w");
        //Form1->printfs("=== COMPILE PL0 ===");
        printfs("=== COMPILE PL0 ===");
        fprintf(FOUT,"=== COMPILE PL0 ===\n");
        ERR=0;
        CC=0; CX=0; LL=0; CH=' '; GetSym();
        if (SYM!=PROGSYM) Error(0);
        else {
            GetSym();
            if (SYM!=IDENT) Error(0);
            else {
                GetSym();
                if (SYM!=SEMICOLON) Error(5);
                else GetSym();
            }
        }
        Block(0,0,SymSetAdd(PERIOD,SymSetUnion(DECLBEGSYS,STATBEGSYS)));
        if (SYM!=PERIOD) Error(9);
        if (ERR==0) Interpret();
        else {
            //Form1->printfs("ERROR IN PL/0 PROGRAM");
            printfs("ERROR IN PL/0 PROGRAM");
            fprintf(FOUT,"ERROR IN PL/0 PROGRAM");
        }
        fprintf(FOUT,"\n"); fclose(FOUT);
    }
}

