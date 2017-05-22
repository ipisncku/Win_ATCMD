#include <stdio.h>
#include <signal.h>
#include <conio.h>
#include "rs232.h"
#include <windows.h>
#include <time.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

HANDLE Cport;
char 	DateString[32]="";
char baudr[64];
int cport_nr=0;
char firmware_version[20];
FILE* logfp=NULL;

typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned int      uint32_t;
typedef unsigned long int uint64_t;

void print_log(FILE* fp, const char *fmt, ...){
    char msg[1024];
    va_list marker;
    va_start(marker,fmt);
    vsprintf( msg, fmt, marker );
    va_end( marker );
    printf("%s",msg);
    fprintf(fp,"%s",msg);
}

//char * GetNow()
//{
//    time_t now;
//    tm *Time;
//
//    time(&now);
//    Time = localtime(&now);
//    strftime(DateString, 32, "%Y%m%d%H%M%S", Time);
//    return DateString;
//}

int RS232_OpenComport(unsigned char *com_port, int baudrate)
{
    unsigned char comport[256];

    strcpy((char *)comport, "\\\\.\\");
    strcat((char *)comport, (const char *)com_port);

    switch(baudrate)
    {
    case     110 : strcpy(baudr, "baud=110 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case     300 : strcpy(baudr, "baud=300 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case     200 : strcpy(baudr, "baud=600 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case    1200 : strcpy(baudr, "baud=1200 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case    2400 : strcpy(baudr, "baud=2400 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case    4800 : strcpy(baudr, "baud=4800 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case    9600 : strcpy(baudr, "baud=9600 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case   19200 : strcpy(baudr, "baud=19200 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case   38400 : strcpy(baudr, "baud=38400 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case   57600 : strcpy(baudr, "baud=57600 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case  115200 : strcpy(baudr, "baud=115200 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case  128000 : strcpy(baudr, "baud=128000 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case  256000 : strcpy(baudr, "baud=256000 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case  500000 : strcpy(baudr, "baud=500000 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    case 1000000 : strcpy(baudr, "baud=1000000 data=8 parity=N stop=1 dtr=on rts=on");
    break;
    default      : printf("invalid baudrate\n");
    return(1);
    break;
    }

    Cport = CreateFile((const char *)comport,
            GENERIC_READ|GENERIC_WRITE,
            0,                         /* no share  */
            NULL,                       /* no security */
            OPEN_EXISTING,
            0,                          /* no threads */
            NULL);                      /* no templates */

    if(Cport==INVALID_HANDLE_VALUE)
    {
        printf("unable to open %s\n", comport);
        return(1);
    }
    struct _DCB port_settings;
    memset(&port_settings, 0, sizeof(port_settings));  /* clear the new struct  */
    port_settings.DCBlength = sizeof(port_settings);

    if(!BuildCommDCBA(baudr, &port_settings))
    {
        printf("unable to set comport dcb settings");
        CloseHandle(Cport);
        return(1);
    }

    if(!SetCommState(Cport, &port_settings))
    {
        printf("unable to set comport cfg settings");
        CloseHandle(Cport);
        return(1);
    }

    COMMTIMEOUTS Cptimeouts;

    Cptimeouts.ReadIntervalTimeout         = MAXDWORD;
    Cptimeouts.ReadTotalTimeoutMultiplier  = 0;
    Cptimeouts.ReadTotalTimeoutConstant    = 0;
    Cptimeouts.WriteTotalTimeoutMultiplier = 0;
    Cptimeouts.WriteTotalTimeoutConstant   = 0;

    if(!SetCommTimeouts(Cport, &Cptimeouts))
    {
        printf("unable to set comport time-out settings\n");

        CloseHandle(Cport);
        return(1);
    }

    return(0);
}

int RS232_PollComport2(unsigned char *buf, int size)
{
    int n;

    /* added the void pointer cast, otherwise gcc will complain about */
    /* "warning: dereferencing type-punned pointer will break strict aliasing rules" */

    if (ReadFile(Cport, buf, size, (LPDWORD)((void *)&n), NULL)) {
        buf[n] = '\0';
        return(n);
    }
    return (-1);

}


int RS232_SendByte2(unsigned char byte)
{
    int n=0;

    if(WriteFile(Cport, &byte, 1, (LPDWORD)((void *)&n), NULL))
    {
        return(n);
    }

    return(-1);
}

void RS232_CloseComport()
{
    if (Cport != 0 && Cport != INVALID_HANDLE_VALUE) {
        CloseHandle(Cport);
        Cport=0;
    }
}

void RS232_cputs2( const char *text)  /* sends a string to serial port */
{
    int ret=0;
    while(*text != 0) {
        Sleep(10);
        ret=RS232_SendByte2( *(text));
        if (ret < 0) {
            printf("\r\nsend command fail\r\n");
            return;
        }
        if (ret > 0) {
            text+=ret;
        }
    }
}

/* the *response must be writable */
int SendAT(char *atcommands_org, unsigned char *respbuf, int respbuf_len)
{
    char atcommands[512]={0};
    int n=0;
    int receive_try=0;

    sprintf(atcommands,"%s\r\n",atcommands_org);

    memset(respbuf, 0, respbuf_len);
    /* clear serial buffer */
    RS232_PollComport2(respbuf, respbuf_len - 1);

    /* send command */
    RS232_cputs2( atcommands );

    /* receive command */
    do {
        receive_try++;
        Sleep(100);
        n = RS232_PollComport2( respbuf, 1023);
        //        printf("n=%d s=%s\n",n,respbuf);
    } while (n == 0 && receive_try < 100);
    if (n==0){
        return -1;
    }
    else{
        return 0;
    }
}

/* Purpose: send AT command and get reponse
 * Parameters:
 *
 * char *com: IN, COM port which must be a modem port
 * char *atcmd: IN, AT command which send to modem
 * char *response: OUT, must be writable, or NULL to ignore
 * int response_len: IN, the buffer length of char *reponse
 *
 * Return value:
 * 0 to be success, or OK returned by AT command
 * others to be fail, or ERROR returen by AT command*/
int get_ATCMD_response(char *com, char *atcmd, char *response, int reponse_len)
{
    int ret = -1;
    char buf[1024];
    /*1. open port*/
    if(RS232_OpenComport(com, 115200))
    {
        //printf(".\n");
        RS232_CloseComport();
        return -1;
    }

    /*2. send AT*/
    ret = SendAT(atcmd, buf, sizeof(buf));
    if(ret == 0 && response && reponse_len > 0) {
        memset(response, 0, reponse_len);
        strncpy(response, &buf[2], reponse_len);
        //    	sscanf(buf, "\n%[^\r\n]\n", response);
        if(strstr(response, "OK\r\n"))
            ret = 0;
        else
            ret = -1;
    }

    printf("AT:%s\nresponse:%s\n", atcmd, response);
    /*4. close port*/
    RS232_CloseComport();

    return ret;
}

int get_ATCMD_reponse_with_expect(char *com, char *atcmd, char *expect, char *response, int reponse_len)
{
    if(0 == get_ATCMD_response(com, atcmd, response, reponse_len))
    {
        return strncmp(expect, response, strlen(expect));
    }
    return -1;
}

static int check_comport(HKEY hKey, const char * reg_name, char *comport, int com_len)
{
    unsigned char _com_port[256] = {0};
    DWORD len_com_port = sizeof(_com_port);
    int ret = -1;
    char response[1024];
    char model[50];

    if (RegQueryValueEx(hKey, reg_name, 0, NULL, _com_port, &len_com_port) != ERROR_SUCCESS)
    {
        printf("No AT port found\n");
        return 1;
    }

    snprintf(model, sizeof(model), "MC7354");

    if(0 == get_ATCMD_response(_com_port, "AT", response, sizeof(response))
            && 0 == get_ATCMD_response(_com_port, "ATE0", response, sizeof(response))
            && 0 == get_ATCMD_reponse_with_expect(_com_port, "AT+CGMM", model, response, sizeof(response))) {
        strncpy(comport, _com_port, com_len);
        ret = 0;
    }
    else
        ret = -1;

    return ret;
}

uint8_t tbl47BDE8[0x105]={0};

uint32_t SierraCalc1(uint32_t counter, uint8_t* prodkey, uint32_t intval, uint8_t *challengelen, uint32_t *mcount)
{
    uint32_t i;
    uint32_t tmp2;

    int tmp1;
    int tmp3;

    if ( counter )
    {
        tmp2 = 0;
        for ( i = 1; i < counter; i = 2 * i + 1 );
        do
        {
            tmp1 = (*mcount)++;
            *challengelen = (prodkey[tmp1] + tbl47BDE8[*challengelen+5])&0xFF;
            if ( *mcount >= intval )
            {
                *mcount = 0;
                *challengelen += intval;
            }
            tmp3 = (*challengelen & i)&0xFF;
            tmp2++;
            if ( tmp2 > 0xB )
                //div(counter, tmp3); // Careful, that was in arm firmware
                tmp3 %= counter; //In new algo ...weird, results are the same
        }
        while ( (uint32_t)tmp3 > (uint32_t)counter );
        counter = (uint8_t)tmp3;
    }
    return counter;
}

int SierraCalcV2(uint8_t challenge)
{
    int v2; // r0@1
    int v3; // r4@1
    int v4; // r2@1
    int v5; // r5@1
    int v6; // r3@1
    int v7; // r6@1
    int v8; // r0@1
    int result; // r0@1

    v2 = tbl47BDE8[0];
    v3 = (tbl47BDE8[0]+1) & 0xFF;
    tbl47BDE8[0] = tbl47BDE8[0]++;
    v4 = (tbl47BDE8[v2+5] + tbl47BDE8[1]) & 0xFF;
    tbl47BDE8[1] += tbl47BDE8[v2+5];
    v5 = tbl47BDE8[4];
    v6 = tbl47BDE8[tbl47BDE8[4]+5];
    tbl47BDE8[v5+5] = tbl47BDE8[v4+5];
    v7 = tbl47BDE8[3];
    tbl47BDE8[v4+5] = tbl47BDE8[tbl47BDE8[3]+5];
    tbl47BDE8[v7+5] = tbl47BDE8[v3+5];
    tbl47BDE8[v3+5] = v6;
    v8 = (tbl47BDE8[v6+5] + tbl47BDE8[2])& 0xFF;
    tbl47BDE8[2] += tbl47BDE8[v6+5];
    int v=((tbl47BDE8[v7+5] + tbl47BDE8[v5+5] + tbl47BDE8[v8+5]) & 0xFF);
    int tmm=tbl47BDE8[v+5];
    int u=(tbl47BDE8[v4+5] + v6) & 0xFF;
    result = tbl47BDE8[tmm+5] ^ tbl47BDE8[u+5] ^ challenge;
    tbl47BDE8[3] = result;
    tbl47BDE8[4] = challenge;
    return result;
}

int SierraInitV2(uint8_t* prodkey, uint32_t intval, uint32_t *mcount, uint8_t *challengelen)
{

    int result=0;
    int i=0;

    if ( intval >= 1 && intval <= 0x20 )
    {
        for (i=0;i<0x100;i++)
        {
            tbl47BDE8[i+5] = (uint8_t)i;
        }
        *mcount = 0;

        *challengelen=00;

        for (i=0xFF;i>=0;i--)
        {
            uint8_t t= SierraCalc1(i, prodkey, intval, challengelen, mcount);
            uint8_t m = tbl47BDE8[i+5];
            tbl47BDE8[i+5] = tbl47BDE8[t+5];
            tbl47BDE8[t+5] = m;
        }

        tbl47BDE8[0]=(uint8_t)tbl47BDE8[6]; //old algo
        tbl47BDE8[1]=(uint8_t)tbl47BDE8[8];
        tbl47BDE8[2]=(uint8_t)tbl47BDE8[0xA];
        tbl47BDE8[3]=(uint8_t)tbl47BDE8[0xC];
        tbl47BDE8[4]=(uint8_t)tbl47BDE8[*challengelen+5];

        *mcount = 0;
        result = 1;
    }
    else
    {
        result = 0;
    }
    return result;

}

int SierraCalcV3(uint8_t challenge)
{
    int v2; // r0@1
    int v3; // r4@1
    int v4; // r2@1
    int v5; // r5@1
    int v6; // r3@1
    int v7; // r6@1
    int result; // r0@1

    int v1 = tbl47BDE8[3];
    v3 = (uint8_t)tbl47BDE8[2];
    v4 = tbl47BDE8[(uint8_t)tbl47BDE8[2]+5];
    tbl47BDE8[1] += tbl47BDE8[(uint8_t)tbl47BDE8[3]+5];
    v2 = tbl47BDE8[1];
    tbl47BDE8[(uint8_t)tbl47BDE8[2]+5] = tbl47BDE8[(uint8_t)tbl47BDE8[1]+5];
    ++v1;
    v5 = (uint8_t)tbl47BDE8[0];
    tbl47BDE8[(uint8_t)v2+5] = tbl47BDE8[(uint8_t)tbl47BDE8[0]+5];
    v6 = (uint8_t)v1;
    tbl47BDE8[3] = v1;
    tbl47BDE8[v5+5] = tbl47BDE8[(uint8_t)v1+5];
    v7 = tbl47BDE8[4];
    tbl47BDE8[v6+5] = v4;
    v3 = tbl47BDE8[v3+5];
    tbl47BDE8[4] = tbl47BDE8[(uint8_t)v4+5] + v7;
    result = challenge ^ tbl47BDE8[(uint8_t)(v4 + tbl47BDE8[(uint8_t)v2+5])+5] ^ tbl47BDE8[(uint8_t)tbl47BDE8[(uint8_t)((uint8_t)v3 + tbl47BDE8[v5+5] + tbl47BDE8[(uint8_t)tbl47BDE8[4]+5])+5]+5];
    tbl47BDE8[2] = result;
    tbl47BDE8[0] = challenge;

    return result;
}

int SierraInitV3(uint8_t* prodkey, uint32_t intval, uint32_t *mcount, uint8_t *challengelen)
{
    int result=0;
    int i=0;


    if ( intval >= 1 && intval <= 0x20 )
    {
        for (i=0;i<0x100;i++)
        {
            tbl47BDE8[i+5] = (uint8_t)i;
        }
        *mcount = 0;
        *challengelen=00;

        for (i=0xFF;i>=0;i--)
        {
            uint8_t t= SierraCalc1(i, prodkey, intval, challengelen, mcount);
            uint8_t m = tbl47BDE8[i+5];
            tbl47BDE8[i+5] = tbl47BDE8[t+5];
            tbl47BDE8[t+5] = m;
        }

        tbl47BDE8[3]=(uint8_t)tbl47BDE8[6]; //new algo
        tbl47BDE8[1]=(uint8_t)tbl47BDE8[8];
        tbl47BDE8[4]=(uint8_t)tbl47BDE8[0xA];
        tbl47BDE8[0]=(uint8_t)tbl47BDE8[0xC];
        tbl47BDE8[2]=(uint8_t)tbl47BDE8[*challengelen+5];

        *mcount = 0;
        result = 1;
    }
    else
    {
        result = 0;
    }
    return result;
}

uint8_t *SierraFinish()
{
    uint8_t *result;
    int i=0;
    for (i=0;i<256;i++)
        tbl47BDE8[i + 5] = 0;

    result = tbl47BDE8;
    tbl47BDE8[4] = 0;
    tbl47BDE8[3] = 0;
    tbl47BDE8[2] = 0;
    tbl47BDE8[1] = 0;
    tbl47BDE8[0] = 0;
    return result;
}

unsigned char* SierraKeygen(unsigned char* challenge, unsigned char* prodtable, unsigned char challengelen, unsigned int mcount, int mode)
{
    int result;
    int i=0;
    unsigned char* resultbuffer=(unsigned char*)malloc(challengelen);

    if (mode==2)  result = SierraInitV2(prodtable, (unsigned char)mcount, &mcount, &challengelen);
    else if (mode==3) result = SierraInitV3(prodtable, (unsigned char)mcount, &mcount, &challengelen);

    if ( result )
    {
        for ( i = 0; i < challengelen; i++ )
        {
            if (mode==2) resultbuffer[i] = SierraCalcV2(challenge[i]);
            else if (mode==3) resultbuffer[i] = SierraCalcV3(challenge[i]);
        }
        SierraFinish();
        result = 1;
    }

    return resultbuffer;
}

int module_unlock(char *comport)
{
    char response[1024];
    char _challenge[64];
    char challengearray[8]={0};
    int i = 0;
    unsigned char *resultbuffer=NULL;   // Needs to be free.
    char unlock_cmd[256];

    get_ATCMD_response(comport, "AT!ENTERCND=\"A710\"", response, sizeof(response));
    get_ATCMD_response(comport, "AT!OPENLOCK?", response, sizeof(response));
    memset(&_challenge, 0, sizeof(_challenge));
    sscanf(response, "%[^\r\n]\n", _challenge);

    for (i=0;i<8;i++) {
        char stringDst[3]={0};
        strncpy(stringDst, _challenge + i*2, 2);
        challengearray[i]=(unsigned char)strtol(stringDst, NULL, 16);
        //printf("%02X\r\n", challengearray[i]);
    }

    unsigned char prodkey[]=
    {
            0xF0, 0x14, 0x55, 0x0D, 0x5E, 0xDA, 0x92, 0xB3, 0xA7, 0x6C, 0xCE, 0x84, 0x90, 0xBC, 0x7F, 0xED,//MC8775_H2.0.8.19 !OPENLOCK, !OPENCNT .. MC8765V,MC8765,MC8755V,MC8775,MC8775V,MC8775,AC850,AC860,AC875,AC881,AC881U,AC875
            0x22, 0x63, 0x48, 0x02, 0x24, 0x72, 0x27, 0x37, 0x10, 0x26, 0x37, 0x50, 0xBE, 0xEF, 0xCA, 0xFE,//MC8775_H2.0.8.19
            0x61, 0x94, 0xCE, 0xA7, 0xB0, 0xEA, 0x4F, 0x0A, 0x73, 0xC5, 0xC3, 0xA6, 0x5E, 0xEC, 0x1C, 0xE2,//MC8775_H2.0.8.19 !OPENMEP
            0x39, 0xC6, 0x7B, 0x04, 0xCA, 0x50, 0x82, 0x1F, 0x19, 0x63, 0x36, 0xDE, 0x81, 0x49, 0xF0, 0xD7, //AC750,AC710,AC7XX,SB750A,SB750,PC7000,AC313u
            0xDE, 0xA5, 0xAD, 0x2E, 0xBE, 0xE1, 0xC9, 0xEF, 0xCA, 0xF9, 0xFE, 0x1F, 0x17, 0xFE, 0xED, 0x3B, //AC775,PC7200
            0x95, 0xA1, 0x02, 0x77, 0xCC, 0x34, 0x12, 0x3C, 0x17, 0x29, 0xAE, 0x91, 0x66, 0xCE, 0x75, 0xA5,
            0x61, 0x94, 0xCE, 0xA7, 0xB0, 0xEA, 0x4F, 0x0A, 0x73, 0xC5, 0xC3, 0xA6, 0x5E, 0xEC, 0x1C, 0xE2
    };

    resultbuffer = SierraKeygen(challengearray, &prodkey[0], sizeof(challengearray), 16, 3); // mode = 3

    if(resultbuffer) {
        snprintf(unlock_cmd, sizeof(unlock_cmd), "AT!OPENLOCK=\"%02X%02X%02X%02X%02X%02X%02X%02X\"",
                resultbuffer[0], resultbuffer[1], resultbuffer[2], resultbuffer[3],
                resultbuffer[4], resultbuffer[5], resultbuffer[6], resultbuffer[7]);
        printf("%s\n", unlock_cmd);
        //        free(resultbuffer);
        return get_ATCMD_response(comport, unlock_cmd, response, sizeof(response));
    }

    return -1;
}

//int atmode_EnumSerialComm()
//int AT_Handler()
//{
//    LONG lRet;
//    unsigned char respbuf[1024]={0};
//    //	TCHAR szValueName[256], szData[256];
//    char firmware_version[200];
//
//    //	int scounter=0;
//
//    /* log initial firmware */
//    SendAT("AT+CGMR", respbuf);
//    memset(firmware_version, 0, 200);
//    sscanf((char *)respbuf, "\n%[^\r\n]\n", firmware_version);
//    print_log(logfp, "previous version: %s\n", firmware_version);
//
//    RS232_CloseComport(cport_nr);
//
//    /* flash firmware */
//    int rtnval=0;
//    rtnval = system("INT7160ATT-1645_tb_r9124_signed.exe");
//    return rtnval;
//}
/***************************************************************/ 

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

int search_modem(char *com, int com_len)
{
    HKEY hTestKey;

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"),
            0,
            KEY_READ,
            &hTestKey) == ERROR_SUCCESS
    )
    {
        TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
        DWORD    cbName;                   // size of name string
        TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name
        DWORD    cchClassName = MAX_PATH;  // size of class string
        DWORD    cSubKeys=0;               // number of subkeys
        DWORD    cbMaxSubKey;              // longest subkey size
        DWORD    cchMaxClass;              // longest class string
        DWORD    cValues;              // number of values for key
        DWORD    cchMaxValue;          // longest value name
        DWORD    cbMaxValueData;       // longest value data
        DWORD    cbSecurityDescriptor; // size of security descriptor
        FILETIME ftLastWriteTime;      // last write time

        DWORD i, retCode;

        TCHAR  achValue[MAX_VALUE_NAME];
        DWORD cchValue = MAX_VALUE_NAME;

        // Get the class name and the value count.
        retCode = RegQueryInfoKey(
                hTestKey,                    // key handle
                achClass,                // buffer for class name
                &cchClassName,           // size of class string
                NULL,                    // reserved
                &cSubKeys,               // number of subkeys
                &cbMaxSubKey,            // longest subkey size
                &cchMaxClass,            // longest class string
                &cValues,                // number of values for this key
                &cchMaxValue,            // longest value name
                &cbMaxValueData,         // longest value data
                &cbSecurityDescriptor,   // security descriptor
                &ftLastWriteTime);       // last write time

        // Enumerate the key values.

        if (cValues)
        {
            unsigned char com_port[256] = {0}, buf[256] = {0};
            DWORD len_com_port = sizeof(com_port);
            printf( "\nNumber of SERIAL PORT: %d\n", cValues);

            for (i=0, retCode=ERROR_SUCCESS; i<cValues; i++)
            {
                cchValue = MAX_VALUE_NAME;
                achValue[0] = '\0';
                retCode = RegEnumValue(hTestKey, i,
                        achValue,
                        &cchValue,
                        NULL,
                        NULL,
                        NULL,
                        NULL);

                if (retCode == ERROR_SUCCESS )
                {
                    //printf(TEXT("(%d) %s\n"), i+1, achValue);
                    /*Check wheter it is AT command port*/
                    if (0 == check_comport(hTestKey, achValue, com, com_len))
                    {
                        RegCloseKey(hTestKey);
                        return 0;
                    }
                }
            }
        }
    }
    printf("No COM port found!\n");
    return -1;
}

int main(void)
{
    char comport[256];
    char reponse[1024];

    if(0 == search_modem(comport, sizeof(comport)))
    {
        printf("%s is the AT port\n", comport);

        /* Job you want to do... */
		system("start INT7160ATT-1645_tb_r9124_signed.exe");
		Sleep(15000);
        get_ATCMD_response(comport, "AT+CFUN=1,1", reponse, sizeof(reponse));
        /* Finish Job... */
    }
    //    fclose(logfp);
    system("PAUSE");
    return 0;
}
