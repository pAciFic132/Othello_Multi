#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

//socket
#include<signal.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#define MAXLINE 512

void z_handler();// 시그널 처리 함수

char *escapechar = "exit"; // 종료 문자열

int server();

//socket variable

int server_sock;
int client_sock;
int clntlen;
int num;
char sendline[MAXLINE];
char recvline[MAXLINE];
int size;
pid_t fork_ret;
struct sockaddr_in client_addr;
struct sockaddr_in server_addr;
int state;
struct sigaction act;

char getData(void);
int getDataCount=0;


///client

#define HIGHT 8
#define WIDTH 8
#define dx 2
#define dy 1

typedef enum Cell {Blank, Black, White} Cell;
typedef enum Menu { Solo, Duo, Manual, Exit } Menu;

void changeGameInputTurn();
void initDisplay();
void inputMenu(Menu *);
void show_Manual();
void inputPlayerName(char player[2][30]);

void getEnableCells(bool, Cell[HIGHT][WIDTH], bool[HIGHT][WIDTH]);
bool existEnableCells(bool[WIDTH][HIGHT]);
bool canPutLine(bool, int[2], int, Cell[HIGHT][WIDTH]);
void add(int[2], int[2]);
bool canPutLineIter(bool, int[2], int, Cell[HIGHT][WIDTH], bool);
bool fillBoard(Cell[HIGHT][WIDTH]);
void finishGame(Cell[HIGHT][WIDTH], char player[2][30]);
bool isOneColor(Cell[HIGHT][WIDTH]);
void inputCell(int selectedCell[2], int *flag);
bool canPut(int[2], bool[WIDTH][HIGHT]);
void reverse(bool, int[2], Cell[HIGHT][WIDTH]);
void reverseIter(bool, int[2], int, Cell[WIDTH][HIGHT]);
void displayBoard(Cell[HIGHT][WIDTH], bool enableCells[WIDTH][HIGHT]);
Cell getStoneColor(bool);
void gotoxy(int, int);
void set_cr_noecho_mode(void);
void tty_mode(int);
void execute(char **);
void execute_m(char **);
void selectByAI(int[2], bool[WIDTH][HIGHT]);
void currentScore(Cell board[HIGHT][WIDTH], char player[2][30]);



int main(int argc, char *argv[]) {
    tty_mode(0);
    set_cr_noecho_mode();
    
    while(true)
    {
        Cell board[HIGHT][WIDTH];
        bool enableCells[WIDTH][HIGHT] = {0};
        char *arglist[2] = {"clear", NULL};
        bool isFirst = true, isAI = true;
        bool isSecond = true;
        int flag = 0; // 게임 도중 'z'키를 누르면 1, 아니면 0
        char player[2][30];
        int enable_count = 0;
        Menu Menu_input = 0;
        
        for ( int j=0; j < HIGHT; j++) {
            for( int i=0; i < WIDTH ; i++) {
                board[i][j] = Blank;
            }
        }
        board[3][3] = White;
        board[4][4] = White;
        board[3][4] = Black;
        board[4][3] = Black;
        
        if( isFirst ){
            isSecond = false;
            isAI = false;
        }
        
        //init server socket
        
        execute(arglist);
        initDisplay();
        inputMenu(&Menu_input);
        
        execute(arglist);
        
        if ( Menu_input == Exit ){ // 나가기
            break;
        }
        
        if ( Menu_input == Manual ){ // 메뉴얼 보기
            show_Manual();
            continue;
        }
        
        if( Menu_input == Duo ){ // 둘이서 하기
            //서버 소켓 기다림
            server();
            execute(arglist);
            inputPlayerName(player);
            execute(arglist);
        }
        
        if( Menu_input == Solo){
            strcpy(player[0], "Player");
            strcpy(player[1], "Computer");
        }
        
        while(true) {
            enable_count = 0;
            
            for (int x=0; x<WIDTH; x++) {
                for (int y=0; y<HIGHT; y++) {
                    enableCells[x][y] = false;
                }
            }
            getEnableCells(isFirst, board, enableCells);// 놓을 수 있는 위치를 찾는다
            displayBoard(board, enableCells); // 판을 보여준다
            currentScore(board, player);
            
            int selectedCell[2] = {-1, -1};
            int checkPossible=0;
            if(!existEnableCells(enableCells)) {
                
                // 종료조건
                if(fillBoard(board)){ // 종료조건 1. 판이 다 찼을 때
                    finishGame(board,player);
                    printf("\n\n\n<<Press Any Key if you want to go Menu>>\n");
                    getchar();//
                    break;
                }
                if (isOneColor(board)) { // 종료조건 2. 판에 한가지 색깔 밖에 없을 때
                    finishGame(board,player);
                    printf("\n\n\n<<Press Any Key if you want to go Menu>>\n");
                    getchar();//
                    break;
                }
                
            }  else{
                if ( Menu_input == Solo ){
                    if ( isAI ){
                        selectByAI(selectedCell, enableCells);
                    }else{
                        printf("human phase : white\n");
                        printf("\n\n\n\n<If you want to quit, You press 'z' Or 'Z'>\n");
                        inputCell(selectedCell, &flag);
                        if ( flag )
                            break;
                        if (!canPut(selectedCell, enableCells)) { // 놓을 수 없는 곳에 두었을 시 기회를 다시 준다
                            printf("Error: 놓을 수 없는 곳을 두셨습니다. 다시 두십시오\n");
                            selectedCell[0] = -1;
                            selectedCell[1] = -1;
                            isFirst = !isFirst;
                            isAI = !isAI;
                        }
                    }
                }
                if ( Menu_input == Duo){ // 둘이서 하기
                    //                    printf("\nwhite : %s\nblack : %s\n",player[0], player[1]);
                    //printf("duo mode %d\n",getDataCount);
                    if( isFirst ){
                        printf("\n\nThis turn is white phase: \n");
                    }
                    else{
                        printf("\n\nThis turn is black phase\n");
                    }
                    printf("\n\n\n\n<If you want to quit, You press 'z' Or 'Z'>\n");
                    inputCell(selectedCell,&flag); // 입력 받는 부분
                    //printf("ok?");
                    if ( flag )
                        break;
                    
                    if (!(checkPossible=canPut(selectedCell, enableCells))) { // 놓을 수 없는 곳에 두었을 시 기회를 다시 준다
                        printf("Error: 놓을 수 없는 곳을 두셨습니다. 다시 두십시오\n");
                        selectedCell[0] = -1;
                        selectedCell[1] = -1;
                        isFirst = !isFirst;
                        isSecond = !isSecond;
                        checkPossible=1;
                    }
                }
            }
            
            //상대방이 입력하도록 바꾸고
            if(checkPossible!=0&&Menu_input==Duo)
            {
                getDataCount++;
                //printf("a");
                //changeGameInputTurn();
            }
            execute(arglist); // clear 실행
            //printf("b");
            reverse(isFirst, selectedCell, board); // 돌을 뒤집는다
            //printf("c");
            isFirst = !isFirst; // 차례를 넘겨준다
            //printf("d");
            isSecond = !isSecond;
            //printf("e");
            isAI = !isAI;
            //printf("f");
            checkPossible=0;
            //printf("g");
        }
        
    }
    
    tty_mode(1);
    return 0;
}

void initDisplay(){
    printf("\n ************************\n");
    printf("         Othello\n");
    printf(" ************************\n");
    printf("\n     ☞  혼자하기\n");
    printf("\n     ☞  같이하기\n");
    printf("\n     ☞  사용법\n");
    printf("\n     ☞  끝내기\n");
}

void inputMenu(Menu *M){
    int x,y;
    int ch1, ch2, ch3;
    *M = 0;
    x = 6;
    y = 6;
    
    gotoxy(x,y);
    
    while(true){
        ch1 = getchar();
        if( ch1 == ' ')
            break;
        if ( ch1 == 27 )
        {
            ch2 = getchar();
            if ( ch2 == 91 )
            {
                ch3 = getchar();
                switch( ch3 ){
                    case 65 :
                        if ( *M >= 1 ){
                            y -= 2;
                            (*M)--;
                            gotoxy(x, y);
                            break;
                        }
                        break;
                    case 66 :
                        if ( *M <= 2 ){
                            y += 2;
                            (*M)++;
                            gotoxy(x, y);
                            break;
                        }
                        break;
                    default:
                        break;
                }
                
            }
        }
    }
    
}

void show_Manual2(){
    FILE *fp;
    char str[50];
    char c;
    
    gotoxy(0,0);
    fp = fopen("Manual.txt","r");
    
    while( !feof(fp) ){
        fgets(str,50,fp);
        printf("%s",str);
    }
    
    printf("<Press Any Key if you can go to Menu again>\n");
    c = getchar();
}

void show_Manual(){
    char *arg[3];
    char c;
    
    arg[0] = "more";
    arg[1] = "Manual.txt";
    arg[2] = NULL;
    
    execute_m(arg);
    
    printf("<Press Any Key if you can go to Menu again>\n");
    c = getchar();
}







void inputPlayerName(char player[2][30]){
    tty_mode(1);
    
    gotoxy(10,3);
    
    printf("Input Player1's Name first\n");
    
    gotoxy(10,5);
    printf("Player1 : ");
    gotoxy(20,5);
    //server get data for stdin first
    fgets(player[0],30,stdin);
    //send name to client
    write(client_sock,player[0],30);
    player[0][strlen(player[0])-1] = '\0';
    
    gotoxy(10,7);
    printf("Player2 : ");
    gotoxy(20,7);
    //get name from the socket
    //    fgets(player[1],30,stdin);
    read(client_sock,player[1],30);
    printf("%s",player[1]);
    printf("%s",player[0]);
    player[1][strlen(player[1])-1] = '\0';
    
    set_cr_noecho_mode();
}

void getEnableCells(bool isFirst, Cell board[HIGHT][WIDTH], bool enableCells[WIDTH][HIGHT]) {
    for (int x=0; x<WIDTH; x++) {
        for (int y=0; y<HIGHT; y++) {
            int selectedCell[2] = {x, y};
            for(int i=0; i<8; i++) {
                if (board[x][y] == Blank
                    && canPutLine(isFirst, selectedCell, i, board)) {
                    enableCells[x][y] = true;
                }
            }
        }
    }
}

bool canPutLine(bool isFirst, int selectedCell[2], int directionIndex, Cell board[HIGHT][WIDTH]) {
    return canPutLineIter(isFirst, selectedCell, directionIndex, board, false);
}


bool canPutLineIter(bool isFirst, int selectedCell[2], int directionIndex, Cell board[HIGHT][WIDTH],
                    bool flag) {
    int directions[8][2] = {
        {-1,-1}, {0,-1}, {1,-1},
        {-1, 0}, {1, 0},
        {-1,1}, {0,1}, {1,1}
    };
    
    int tmp[2] = {selectedCell[0], selectedCell[1]};
    add(tmp, directions[directionIndex]);
    
    Cell myColor = getStoneColor(isFirst);
    if (tmp[0] <= -1 || tmp[1] <= -1 || tmp[0] > WIDTH || tmp[1] > HIGHT) {
        return false;
    } else if (board[tmp[0]][tmp[1]] == Blank) {
        return false;
    }
    
    if (board[tmp[0]][tmp[1]] != myColor && !flag) {
        return canPutLineIter(isFirst, tmp, directionIndex, board, true);
    } else if (board[tmp[0]][tmp[1]] == myColor && !flag) {
        return false;
    }  else if (board[tmp[0]][tmp[1]] == myColor && flag) {
        return true;
    }
    return canPutLineIter(isFirst, tmp, directionIndex, board, true);
}

void add(int target[2], int a[2]) {
    target[0] = target[0] + a[0];
    target[1] = target[1] + a[1];
}

bool existEnableCells(bool enableCells[WIDTH][HIGHT]) {
    for (int x=0; x<WIDTH; x++) {
        for (int y=0; y<HIGHT; y++) {
            if (enableCells[x][y]) {
                return true;
            }
        }
    }
    return false;
}

bool fillBoard(Cell board[HIGHT][WIDTH]) {
    for (int y=0; y < HIGHT; y++) {
        for(int x=0; x < WIDTH ; x++) {
            if (board[x][y] == Blank)
                return false;
        }
    }
    return true;
}

void currentScore(Cell board[HIGHT][WIDTH], char player[2][30]) {
    int countblack = 0, countwhite = 0;
    for (int y=0; y < HIGHT; y++) {
        for(int x=0; x < WIDTH ; x++) {
            switch (board[x][y]) {
                case Black:
                    countblack++;
                    break;
                case White:
                    countwhite++;
                    break;
                default:
                    break;
            }
        }
    }
    
    gotoxy(25 , 5 );
    printf("black ●- %s %d",player[1],countwhite);
    gotoxy(25 , 6 );
    printf("white ○ - %s %d",player[0], countblack);
    gotoxy(0, 12);
    
}

void finishGame(Cell board[HIGHT][WIDTH], char player[2][30]) {
    int countblack = 0, countwhite = 0;
    for (int y=0; y < HIGHT; y++) {
        for(int x=0; x < WIDTH ; x++) {
            switch (board[x][y]) {
                case Black:
                    countblack++;
                    break;
                case White:
                    countwhite++;
                    break;
                default:
                    break;
            }
        }
    }
    
    printf("black:%d\n",countwhite);
    printf("white:%d\n",countblack);
    
    printf("\n\n\n\n\n<<Game Over !!>> \n");
    if(countwhite > countblack) {
        printf("winner: black - %s\n",player[1]);
    } else if (countwhite < countblack) {
        printf("winner: white - %s\n",player[0]);
    } else {
        printf("draw\n");
    }
}

bool isOneColor(Cell board[HIGHT][WIDTH]) {
    enum Cell color = Blank;
    for (int y=0; y < HIGHT; y++) {
        for(int x=0; x < WIDTH ; x++) {
            if(board[x][y]!=Blank) {
                if(color == Blank) {
                    color = board[x][y];
                } else if(board[x][y]!=color) {
                    return false;
                }
            }
            if(y == HIGHT-1 && x == WIDTH-1)
                return true;
        }
    }
    return false;
}

void inputCell(int selectedCell[2], int *flag){
    int x,y;
    int ch1, ch2, ch3;
    int xx = 0, yy = 0;
    x = 10;
    y = 6;
    
    gotoxy(x,y);
    while(true)
    {
        //printf("-1");
        ch1 = getData();
        if( ch1 == ' ')
            break;
        if ( ch1 == 'z' || ch1 == 'Z' )
            break;
        if ( ch1 == 27 )
        {
            ch2 = getData();
            if ( ch2 == 91 )
            {
                ch3 = getData();
                switch( ch3 ){
                    case 68 :
                        x -= dx;
                        xx--;
                        gotoxy(x, y);
                        break;
                    case 67 :
                        xx++;
                        x += dx;
                        gotoxy(x, y);
                        break;
                    case 65 :
                        y -= dy;
                        yy--;
                        gotoxy(x, y);
                        break;
                    case 66 :
                        y += dy;
                        yy++;
                        gotoxy(x, y);
                        break;
                    default:
                        break;
                }
                
            }
        }
    }
    
    if ( ch1 == 'z' || ch1 == 'Z'){
        *flag = 1;
    }else{
        selectedCell[0] = 4 + xx -1;
        selectedCell[1] = 4 + yy -1;
    }

    //printf("hmm");
    return;
}
bool canPut(int selectedCell[2], bool enableCells[WIDTH][HIGHT]) {
    return enableCells[selectedCell[0]][selectedCell[1]];
}

void reverse(bool isFirst, int selectedCell[2], Cell board[HIGHT][WIDTH]) {
    int x = selectedCell[0], y = selectedCell[1];
    Cell myColor = getStoneColor(isFirst);
    
    board[x][y] = myColor;
    for (int i=0; i < 8; i++) {
        if (canPutLine(isFirst, selectedCell, i, board)) {
            reverseIter(isFirst, selectedCell, i, board);
        }
    }
}

void reverseIter(bool isFirst, int scanningCell[2], int directionIndex, Cell board[WIDTH][HIGHT]) {
    int directions[8][2] = {
        {-1,-1}, {0,-1}, {1,-1},
        {-1, 0}, {1, 0},
        {-1,1}, {0,1}, {1,1}
    };
    
    int tmp[2] = {scanningCell[0], scanningCell[1]};
    add(tmp, directions[directionIndex]);
    
    int x = tmp[0];
    int y = tmp[1];
    
    Cell myColor = getStoneColor(isFirst);
    
    if (board[x][y] == myColor || board[x][y] == Blank) {
        return;
    }
    
    board[x][y] = myColor;
    reverseIter(isFirst, tmp, directionIndex, board);
}

void displayBoard(Cell board[HIGHT][WIDTH], bool enableCells[WIDTH][HIGHT]) {
    printf("\n");
    printf("   ");
    for(int i = 1; i <= WIDTH; i++){
        printf("%d ", i);
    }
    printf("\n");
    
    for (int y=0; y < HIGHT; y++) {
        printf(" %d ", y+1);
        for(int x=0; x < WIDTH ; x++) {
            switch (board[x][y]) {
                case Black:
                    printf("○ ");
                    break;
                case White:
                    printf("● ");
                    break;
                default:
                    if ( enableCells[x][y] ){
                        printf("☆ ");
                    }
                    else{
                        printf("- ");
                    }
                    
                    break;
            }
        }
        printf("\n");
    }
}

Cell getStoneColor(bool isFirst) {
    if(isFirst) {
        return Black;
    }
    return White;
}

void gotoxy(int x, int y){
    printf("\033[%d;%df",y,x);
    fflush(stdout);
}

void set_cr_noecho_mode(){
    struct termios ttystate;
    
    tcgetattr(0, &ttystate);
    ttystate.c_lflag &= ~ICANON;
    ttystate.c_lflag &= ~ECHO;
    ttystate.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &ttystate);
}

void tty_mode(int how){
    static struct termios original_mode;
    if ( how == 0 )
        tcgetattr(0, &original_mode);
    else
        tcsetattr(0, TCSANOW, &original_mode);
}

void execute( char *arglist[] )
{
    const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
    write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);
}


void selectByAI(int selectedCell[2], bool enableCells[WIDTH][HIGHT]) {
    int temp;
    
    for (int y=0; y < HIGHT; y++) {
        for(int x=0; x < WIDTH ; x++) {
            if(enableCells[x][y]) {
                selectedCell[0] = x;
                selectedCell[1] = y;
            }
        }
    }
}

//socket

int server()
{
    //    int server_sock;
    //    int client_sock;
    //    int clntlen;
    //    int num;
    //    char sendline[MAXLINE];
    //    char recvline[MAXLINE];
    //    int size;
    //    pid_t fork_ret;
    //    struct sockaddr_in client_addr;
    //    struct sockaddr_in server_addr;
    //    int state;
    //    struct sigaction act;
    act.sa_handler = z_handler;
    
    // 소켓 생성
    if((server_sock = socket(PF_INET, SOCK_STREAM, 0)) <0)
    {
        printf("Server : can't open stream socket. \n");
        exit(0);
    }
    // 소켓 주소 구조체에 주소 세팅
    bzero((char *)&server_addr, sizeof(server_addr)); // 소켓 주소 구조체 초기화
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(9120);
    
    sigaction(SIGCHLD, &act, 0);
    
    // 소켓에 서버 주소 연결
    if(bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Server : can't bind local address.\n");
        exit(0);
    }
    printf("Server started. \nWaiting for client.. \n");
    listen(server_sock, 1);
    
    // 클라이언트의 연결요청 수락
    clntlen = sizeof(client_addr);
    if((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &clntlen)) <0)
    {
        printf("Server : failed in accepting. \n");
    }
    
    //    if((fork_ret = fork()) > 0)
    //    {
    //        // 부모 프로세스는 키보드 입력을 클라이언트로 송신
    //        while(fgets(sendline, MAXLINE, stdin)!=NULL)
    //        {
    //            size = strlen(sendline);
    //            if(write(client_sock, sendline, strlen(sendline)) != size)
    //            {
    //                printf("Error in write. \n");
    //            }
    //            if(strstr(sendline, escapechar) != NULL) // 종료 문자열 입력시 처리
    //            {
    //                printf("Good bye.\n");
    //                close(client_sock);
    //                while(1);    //자식프로세서가 죽을때까지 블로킹
    //            }
    //        }
    //    }
    //    else if(fork_ret == 0)
    //    {
    //        // 자식 프로세스는 클라이언트로부터 수신된 메시지를 화면에 출력
    //        while(1)
    //        {
    //            if((size = read(client_sock, recvline, MAXLINE)) < 0)
    //            {
    //                printf("Error if read. \n");
    //                close(client_sock);
    //                exit(0);
    //            }
    //            recvline[size] = '\0';
    //            if(strstr(recvline, escapechar) != NULL) // 종료 문자열 입력시 처리
    //            {
    //                write(client_sock, escapechar, strlen(escapechar));
    //                break;
    //            }
    //            printf("%s", recvline); // 화면 출력
    //        }
    //    }
    
    return 0;
}
void z_handler()
{
    int state;
    waitpid(-1, &state, WNOHANG);
    exit(0);
    
    return ;
}
int count=0;
void GameInputTurn(){
    //server -> client
    if(count%2==0){
        int fd=open(ttyname(STDIN_FILENO),O_RDONLY);
        dup2(fd,0);
        dup2(fd,client_sock);
    }
    else{//client -> server
        dup2(client_sock,0);
    }
    count++;
}

char getData(void){
    // printf("getDataCount %d",getDataCount);
    if(getDataCount%2==0){
        char data[2];
        data[0]=getchar();
        //send data to client
        write(client_sock,data,2);
        return data[0];
    }
    else{
        //get data from client
        char data[2];
        read(client_sock,data,2);
        return data[0];
    }
}




void execute_m( char *arglist[] )
{
    int pid,exitstatus;
    
    pid = fork();
    switch( pid ){
        case -1:
            perror("fork failed");
            exit(1);
        case 0:
            execvp(arglist[0], arglist);
            perror("execvp failed");
            exit(1);
        default:
            while( wait(&exitstatus) != pid )
                ;
    }
}
