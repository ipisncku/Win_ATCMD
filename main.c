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
    	snprintf(response, reponse_len, &buf[2]);
//    	sscanf(buf, "\n%[^\r\n]\n", response);
	}

	printf("AT:%s\nresponse:%s\n", atcmd, response);
    /*4. close port*/
    RS232_CloseComport();

    return 0;
}

int get_ATCMD_reponse_with_expect(char *com, char *atcmd, char *expect, char *response, int reponse_len)
{
    if(0 == get_ATCMD_response(com, atcmd, response, reponse_len))
    {
        return strncmp(expect, response, reponse_len);
    }
    return -1;
}

static int check_comport(HKEY hKey, const char * reg_name, char *comport, int com_len)
{
	unsigned char com_port[256] = {0};
	DWORD len_com_port = sizeof(com_port);
	int ret = -1;
	char response[1024];
	
	if (RegQueryValueEx(hKey, reg_name, 0, NULL, com_port, &len_com_port) != ERROR_SUCCESS)
	{
		printf("No AT port found\n");
		return 1;
	}
		
	if(0 == get_ATCMD_response(com_port, "AT", response, sizeof(response))) {
		strncpy(comport, com_port, com_len);
		ret = 0;
	}
	else
		ret = -1;
	
	return ret;
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
        get_ATCMD_response(comport, "AT+CGSN", reponse, sizeof(reponse));
        get_ATCMD_response(comport, "AT+CSQ", reponse, sizeof(reponse));
    }
    fclose(logfp);
    return 0;
}
