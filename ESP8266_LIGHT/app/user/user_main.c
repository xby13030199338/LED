#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "gpio.h"
#include "uart.h"
#include "pwm.h"
#include "hw_timer.h"
#include "alasmart/alacloud.h"
#include "alasmart/aipwm.h"


struct sockaddr_in server_addr, client_addr;
socklen_t sin_size;

#define SPI_FLASH_SEC_SIZE 4096
#define server_ip "www.trencon.com"
#define server_port 9999

xTaskHandle xHandleConnect=NULL;
xTaskHandle xHandleWrite=NULL;
xTaskHandle xHandleLocal=NULL;
xTaskHandle xHandleBlink=NULL;

char recv_buf[1];
char package_data[200];
char package_device_data[200];

char ala_send[200]={0};
bool remote_connected=false;

char wifiid[200];
char wifipwd[50];
char wifiuuid[50];
char phoneuuid[50];

bool local_connected=false;
int server_sock=-1;
int sta_socket=-1;
uint8 version=1;
char ip[20];
char platform[200];
char rgb_value[50];
char state_value[50];
char rgblevel_value[50];
char wlevel_value[50];
char on_value[50];
char firmware_value[50];


typedef void ETSTimerFunc(void *timer_arg);
os_timer_t mytimer;

uint16 red=0;
uint16 green=255;
uint16 blue=0;
uint16 rgblevel=16;
uint16 wlevel=0;
uint8 on=1;
uint32 alltimes=0;

uint8 smart=0;

char blink_value[200];
uint8 blink_vals[50];
uint32 blink_time=0;
uint32 blink_timei=0;

uint8 deepsleep=0;


void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
	int len=0;
	uint8 i=0;
	uint8 j=0;
    switch(status) {
        case SC_STATUS_WAIT:
            printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            printf("SC_STATUS_GETTING_SSID_PSWD\n");
            sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
        	printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = pdata;
	        sprintf(wifiid,sta_conf->ssid);
	        sprintf(wifipwd,sta_conf->password);

	        printf("wifipwd=%s\n",wifipwd);

	        len=strlen(wifipwd);

	        j=0;
			for(i=len-16;i<len;i++)
			{
				wifiuuid[j]=wifipwd[i];
				j++;
			}
			wifiuuid[j]=0;
			ala_pwd(wifiuuid);


			j=0;
			for(i=len-30;i<len-16;i++)
			{
				phoneuuid[j]=wifipwd[i];
				j++;
			}
			phoneuuid[j]=0;
			printf("phoneuuid=%s\n",phoneuuid);


			printf("wifiuuid=%s\n",wifiuuid);

	        wifiuuid[0]='1';
	        wifiuuid[1]=',';
	        j=0;
	        for(i=len-16;i<len;i++)
	        {
	        	wifiuuid[j+2]=wifipwd[i];
	        	j++;
	        }
	        wifiuuid[j+2]=',';
	        wifiuuid[j+3]='1';
	        wifiuuid[j+4]=0;

	        printf("wifiuuid=%s\n",wifiuuid);

	        memcpy(sta_conf->password,wifipwd,len-30);
	        sta_conf->password[len-30]=0;

	        spi_flash_erase_sector(0x52);
			uint32 writelen=strlen(wifiuuid);
			uint8 header[4]={0xEF,0xEF,0xEF,0xEF};
			spi_flash_write(0x52*4*1024,(uint32*)header, 4);
			spi_flash_write(0x52*4*1024+4,&writelen, 4);
			spi_flash_write(0x52*4*1024+8,(uint32*)wifiuuid, strlen(wifiuuid));
			uint32 c=0x01010101;
			spi_flash_write(0x52*4*1024+190,&c, 4);

	        printf("wifipwd=%s\n",sta_conf->password);
	        wifi_station_set_config(sta_conf);
	        wifi_station_disconnect();
	        wifi_station_connect();



            break;
        case SC_STATUS_LINK_OVER:
            printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};
                memcpy(phone_ip, (uint8*)pdata, 4);
                printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            }
            smartconfig_stop();



            break;
    }

}


void ICACHE_FLASH_ATTR
smartconfig_task(void *pvParameters)
{
	wifi_station_dhcpc_start();

	spi_flash_erase_sector(0x54);
	uint8 header[4]={0x00,0x00,0x00,0x00};
	spi_flash_write(0x54*4*1024,(uint32*)header, 4);

    smartconfig_start(smartconfig_done);
    smart=1;
    aipwm_set(0, 3000);
    aipwm_set(1, 0);
    aipwm_set(2, 0);
    aipwm_set(3, 0);
    vTaskDelay(500/portTICK_RATE_MS);
    aipwm_set(0, 0);
	aipwm_set(1, 0);
	aipwm_set(2, 0);
	aipwm_set(3, 0);
	vTaskDelay(500/portTICK_RATE_MS);

	aipwm_set(0, 3000);
	aipwm_set(1, 0);
	aipwm_set(2, 0);
	aipwm_set(3, 0);
	vTaskDelay(500/portTICK_RATE_MS);
	aipwm_set(0, 0);
	aipwm_set(1, 0);
	aipwm_set(2, 0);
	aipwm_set(3, 0);
	vTaskDelay(500/portTICK_RATE_MS);

	aipwm_set(0, 3000);
	aipwm_set(1, 0);
	aipwm_set(2, 0);
	aipwm_set(3, 0);
	vTaskDelay(500/portTICK_RATE_MS);
	aipwm_set(0, 0);
	aipwm_set(1, 0);
	aipwm_set(2, 0);
	aipwm_set(3, 0);
	vTaskDelay(500/portTICK_RATE_MS);

	aipwm_set(0, 3000);
	aipwm_set(1, 0);
	aipwm_set(2, 0);
	aipwm_set(3, 0);

    vTaskDelete(NULL);
}



char* get_deviceid()
{
	char sta_mac[6];
	wifi_get_macaddr(STATION_IF, sta_mac);
	char *deviceid=zalloc(50);
	sprintf(deviceid,"%02x%02x%02x%02x%02x%02x",sta_mac[0],
	        sta_mac[1],sta_mac[2],sta_mac[3],
	    	sta_mac[4],sta_mac[5]);

	return deviceid;
}

char* get_devicetype()
{
	return "led_light_01";
}

int sock_id=-1;

void task_local_read(void *pvParameters)
{
	bool ala_set_rgb(char* value);
	bool ala_set_on(char* value);

	bool ala_set_blinkval(char *value);
	bool ala_set_blinktime(char *value);

	int client_sock=sock_id;

	int timeout=3;
	lwip_setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

	printf("S > client_sock=%d\n",client_sock);

	char package_type=0;
	uint8 package_start_size=0;//锟斤拷始锟斤拷歉锟斤拷锟�
	uint16 package_all_size=0;//锟斤拷锟斤拷锟街斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷始锟斤拷呛捅锟斤拷锟�
	uint16 package_all_datai=0;
	uint8 package_all_hsize=0;
	uint8 package_all_lsize=0;

	uint16 package_size=0;
	uint16 package_datai=0;
	uint8 package_hsize=0;
	uint8 package_lsize=0;

	uint16 package_device_size=0;
	uint16 package_device_datai=0;
	uint8 package_device_hsize=0;
	uint8 package_device_lsize=0;

	char *local_recv_buf=zalloc(1);
	char *local_package_data=zalloc(300);
	char *local_package_device_data=zalloc(100);

	char res[3] = "ok!";
	char command_flag;

	while (read(client_sock , local_recv_buf, 1)>0)
	{
		if (local_recv_buf[0] == '#' && package_start_size < 2)
		{
			package_start_size++;

		}

		else if (local_recv_buf[0] == '/' && package_start_size == 2)
		{
			package_start_size = 0;
			package_datai = 0;
			command_flag = 1;

		}

		else if (package_start_size == 2)
		{
			local_package_data[package_datai]=local_recv_buf[0];
			package_datai++;

		}

		else
		{
			package_start_size = 0;
			package_datai = 0;
		}

		if (local_recv_buf[0] == '/' && command_flag)
		{
			//printf("%s\n", local_package_data);

			if (!strcmp (local_package_data, "red  "))
			{
				ala_set_rgb("255#0#0");
				wlevel = 0;
				ala_set_on("1");

				write(client_sock, res, 3);
				command_flag = 0;

			}

			if (!strcmp (local_package_data, "green"))
			{
				ala_set_rgb("0#255#0");
				wlevel = 0;
				ala_set_on("1");

				write(client_sock, res, 3);
				command_flag = 0;

			}

			if (!strcmp (local_package_data, "blue "))
			{
				ala_set_rgb("0#0#255");
				wlevel = 0;
				ala_set_on("1");

				write(client_sock, res, 3);
				command_flag = 0;

			}

			if (!strcmp (local_package_data, "all  "))
			{
				ala_set_rgb("255#255#255");
				wlevel = 16;
				ala_set_on("1");

				write(client_sock, res, 3);
				command_flag = 0;

			}

			if (!strcmp (local_package_data, "light"))
			{
				ala_set_rgb("0#0#0");
				wlevel = 16;
				ala_set_on("1");

				write(client_sock, res, 3);
				command_flag = 0;

			}

			if (!strcmp (local_package_data, "close"))
			{
				ala_set_rgb("0#0#0");
				wlevel = 0;
				ala_set_on("1");

				write(client_sock, res, 3);
				command_flag = 0;

			}

			if (!strcmp (local_package_data, "show "))
			{
				ala_set_rgb("255#255#255");
				wlevel = 0;
				ala_set_on("1");

				write(client_sock, res, 3);
				command_flag = 0;
			}

		}







		//printf("1\n");


		/*
		if(local_recv_buf[0]==0xEF&&package_start_size<5){
			package_start_size++;
			printf("2\n");
		}
		else if(package_type==0&&package_start_size==5){
			package_type=local_recv_buf[0];
			printf("S > package_type=%d\n",package_type);
			printf("3\n");
		}
		else if(package_all_lsize==0&&package_start_size==5){
			package_all_lsize=local_recv_buf[0];
			printf("4\n");

		}
		else if(package_all_size==0&&package_start_size==5){
			package_all_hsize=local_recv_buf[0];
			package_all_size=package_all_hsize*256+package_all_lsize;
			printf("5\n");

		}
		else if(package_device_lsize==0&&package_start_size==5){
			package_device_lsize=local_recv_buf[0];
			printf("6\n");

		}
		else if(package_device_size==0&&package_start_size==5){
			package_device_hsize=local_recv_buf[0];
			package_device_size=package_device_hsize*256+package_device_lsize;
			printf("7\n");

		}
		else if(package_lsize==0&&package_start_size==5){
			package_lsize=local_recv_buf[0];
			printf("8\n");

		}
		else if(package_size==0&&package_start_size==5){
			package_hsize=local_recv_buf[0];

			package_size=package_hsize*256+package_lsize;

			printf("9\n");

		}
		else if(package_device_datai<package_device_size&&package_start_size==5)
		{
			local_package_device_data[package_device_datai]=local_recv_buf[0];
			package_device_datai++;
			printf("10\n, package_device_datai : %d, package_device_size : %d", package_device_datai, package_device_size);

			if(package_device_datai==package_device_size)
			{
				local_package_device_data[package_device_size]=0;
				printf("11\n");
			}
		}
		else if(package_start_size==5){
			local_package_data[package_datai]=local_recv_buf[0];
			package_datai++;
			printf("12\n");

			if(package_datai==package_size){

				int retsize=ala_parse_receive(1,local_package_device_data,package_type,package_size,local_package_data);
				printf("retsize is=%d \r\n",retsize);
				if(retsize>0)
				{


					if (write(client_sock, local_package_data, retsize) < 0) {
						printf("S > local send fail\n");
						break;

					}else{
						printf("S > local send ok\n");
					}
				}

				printf("local send is end \r\n");
				package_type=0;
				package_start_size=0;
				package_all_size=0;
				package_all_datai=0;
				package_all_hsize=0;
				package_all_lsize=0;

				package_size=0;
				package_hsize=0;
				package_lsize=0;
				package_datai=0;

				package_device_size=0;
				package_device_hsize=0;
				package_device_lsize=0;
				package_device_datai=0;
				//break;
			}
		}
		else
		{
			package_type=0;
			package_start_size=0;
			package_all_size=0;
			package_all_datai=0;
			package_all_hsize=0;
			package_all_lsize=0;

			package_size=0;
			package_hsize=0;
			package_lsize=0;
			package_datai=0;

			package_device_size=0;
			package_device_hsize=0;
			package_device_lsize=0;
			package_device_datai=0;
		}

		*/
	}





	closesocket(client_sock);
	shutdown(client_sock,2);

	free(local_recv_buf);
	free(local_package_data);
	free(local_package_device_data);
	printf("S > close client\n");


	vTaskDelete(NULL);
}


void task_local(void *pvParameters)
{


	while (1) {

		int client_sock=-1;
		if(server_sock!=-1)
		{
			shutdown(server_sock,2);
			close(server_sock);
		}
		if (-1 == (server_sock = socket(AF_INET, SOCK_STREAM, 0))) {
			printf("S > socket error\n");
			break;
		}
		printf("S > create socket: %d\n", server_sock);
		if (-1 == bind(server_sock, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr))) {
			printf("S > bind fail\n");
			break;
		}
		printf("S > bind port: %d\n", ntohs(server_addr.sin_port));
		if (-1 == listen(server_sock, 5)) {
			printf("S > listen fail\n");
			break;
		}
		printf("S > listen ok\n");
		sin_size = sizeof(client_addr);

		local_connected=true;

		while(1){
			printf("S > wait client\n");


			if ((client_sock= accept(server_sock, (struct sockaddr *) &client_addr, &sin_size)) < 0) {
				printf("S > accept fail\n");
				continue;
			}
			printf("S > my client from %s %d\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));
			sock_id=client_sock;
			xTaskCreate(task_local_read, "tsk_local_read", 256, NULL, 2, NULL);

		}
		printf("S > end accept\n");

	}


}


void remote_read()
{

	printf("Start Read %d!\r\n",sta_socket);

	printf("rgb:%d,%d,%d,%d,%d,%d",rgblevel,wlevel,red*rgblevel-1,green*rgblevel-1,blue*rgblevel-1,256*wlevel-1);

	char package_type=0;
	uint8 package_start_size=0;//锟斤拷始锟斤拷歉锟斤拷锟�
	uint16 package_all_size=0;//锟斤拷锟斤拷锟街斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷始锟斤拷呛捅锟斤拷锟�
	uint16 package_all_datai=0;
	uint8 package_all_hsize=0;
	uint8 package_all_lsize=0;

	uint16 package_size=0;
	uint16 package_datai=0;
	uint8 package_hsize=0;
	uint8 package_lsize=0;

	uint16 package_device_size=0;
	uint16 package_device_datai=0;
	uint8 package_device_hsize=0;
	uint8 package_device_lsize=0;


	while (read(sta_socket , recv_buf, 1)>0) {
		if(recv_buf[0]==0xEF&&package_start_size<5){
			package_start_size++;

		}
		else if(package_type==0&&package_start_size==5){
			package_type=recv_buf[0];
		}
		else if(package_all_lsize==0&&package_start_size==5){
			package_all_lsize=recv_buf[0];

		}
		else if(package_all_size==0&&package_start_size==5){
			package_all_hsize=recv_buf[0];
			package_all_size=package_all_hsize*256+package_all_lsize;

		}
		else if(package_device_lsize==0&&package_start_size==5){
			package_device_lsize=recv_buf[0];

		}
		else if(package_device_size==0&&package_start_size==5){
			package_device_hsize=recv_buf[0];
			package_device_size=package_device_hsize*256+package_device_lsize;

		}
		else if(package_lsize==0&&package_start_size==5){
			package_lsize=recv_buf[0];

		}
		else if(package_size==0&&package_start_size==5){
			package_hsize=recv_buf[0];
			package_size=package_hsize*256+package_lsize;

		}
		else if(package_device_datai<package_device_size&&package_start_size==5)
		{
			package_device_data[package_device_datai]=recv_buf[0];
			package_device_datai++;
			if(package_device_datai==package_device_size)
			{
				package_device_data[package_device_size]=0;

			}
		}
		else if(package_start_size==5){

			package_data[package_datai]=recv_buf[0];
			package_datai++;
			if(package_datai==package_size){


				int retsize=ala_parse_receive(0,package_device_data,package_type,package_size,package_data);
				printf("C > ala_parse_receive %d\n",retsize);

				if(retsize>0)
				{
					if (write(sta_socket, package_data, retsize) < 0) {
						printf("C > remote send fail\n");
					}else{
						printf("C > remote send ok %d\n",retsize);
					}
				}
				else if(retsize==-1)
				{
					system_restart();
					return;
				}

				package_type=0;
				package_start_size=0;
				package_all_size=0;
				package_all_datai=0;
				package_all_hsize=0;
				package_all_lsize=0;

				package_size=0;
				package_hsize=0;
				package_lsize=0;
				package_datai=0;

				package_device_size=0;
				package_device_hsize=0;
				package_device_lsize=0;
				package_device_datai=0;
				printf("remote send is end \r\n");
			}
		}
		else
		{
			package_type=0;
			package_start_size=0;
			package_all_size=0;
			package_all_datai=0;
			package_all_hsize=0;
			package_all_lsize=0;

			package_size=0;
			package_hsize=0;
			package_lsize=0;
			package_datai=0;

			package_device_size=0;
			package_device_hsize=0;
			package_device_lsize=0;
			package_device_datai=0;
		}
	}

	printf("End Read.........!\r\n");

	close(sta_socket);

	remote_connected=false;

}

void task_connect(void *pvParameters)
{



	while (1) {

		if(!remote_connected){



			struct sockaddr_in remote_ip;
			bzero(&remote_ip, sizeof(struct sockaddr_in));
			remote_ip.sin_family = AF_INET;

			struct ip_addr addr;


			netconn_gethostbyname(server_ip,&addr);

			while(addr.addr==0){
				vTaskDelay(200 / portTICK_RATE_MS);
				netconn_gethostbyname(server_ip,&addr);
			}

			remote_ip.sin_addr.s_addr = addr.addr;
			remote_ip.sin_port = htons(server_port);

			int keepAlive = 1;
			int keepIdle = 60;//in seconds
			int keepInterval = 4;//in seconds
			int keepCount = 2;
			int timeout=5000;

			printf("remote_connect start.....!\n");

			sta_socket = socket(PF_INET, SOCK_STREAM, 0);
			lwip_setsockopt(sta_socket,SOL_SOCKET,SO_KEEPALIVE,(void*)&keepAlive,sizeof(keepAlive));
			lwip_setsockopt(sta_socket,IPPROTO_TCP,TCP_KEEPIDLE,(void *)&keepIdle,sizeof(keepIdle));
			lwip_setsockopt(sta_socket,IPPROTO_TCP,TCP_KEEPINTVL,(void *)&keepInterval,sizeof(keepInterval));
			lwip_setsockopt(sta_socket,IPPROTO_TCP,TCP_KEEPCNT,(void *)&keepCount,sizeof(keepCount));
			//lwip_setsockopt(sta_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

			if (-1 == sta_socket) {
				printf("C > socket fail!\n");
			}else{
				printf("C > socket %d ok! \n",sta_socket);
			}

			printf("%s=%s\n",server_ip,inet_ntoa((struct ip_addr){addr.addr}));
			if (0 != connect(sta_socket, (struct sockaddr *)(&remote_ip), sizeof(struct sockaddr))) {
				close(sta_socket);
				printf("C > connect fail!\n");
				vTaskDelay(60000 / portTICK_RATE_MS);
				continue;
			}

			printf("C > connect ok!\n");


			if(xHandleLocal!=NULL){
				printf("C > delete xHandleLocal ok!\n");
				vTaskDelete(xHandleLocal);
			}

			xTaskCreate(task_local, "tsk_local", 256, NULL, 2, &xHandleLocal);


			remote_connected=true;

			short count=ala_connect_platform(platform);

			if (write(sta_socket, platform, count) < 0) {
				printf("C > send fail\n");
			}
			else {
				printf("C > send success %d\n",count);
			}
			if(smart==1)
			{
				aipwm_set(0, 0);
				aipwm_set(1, 4095);
				aipwm_set(2, 0);
				aipwm_set(3, 0);
				smart=0;


				red=0;
				green=255;
				blue=0;
				rgblevel=16;
				wlevel=0;
				on=1;

			}

			remote_read();


		}

	}
}




void wifi_handle_event_cb(System_Event_t *evt)
{

	switch (evt->event_id) {
		case EVENT_STAMODE_CONNECTED:
			printf("connect to ssid %s, channel %d\n",
					evt->event_info.connected.ssid,
					evt->event_info.connected.channel);


			break;

		case EVENT_STAMODE_DISCONNECTED:
			printf("mydisconnect from ssid %s, reason %d\n",
					evt->event_info.disconnected.ssid,
					evt->event_info.disconnected.reason);

			break;
		case EVENT_STAMODE_AUTHMODE_CHANGE:
			printf("mode: %d -> %d\n",
					evt->event_info.auth_change.old_mode,
					evt->event_info.auth_change.new_mode);
			break;
		case EVENT_STAMODE_GOT_IP:
			printf("myip:" IPSTR ",mask:" IPSTR ",mygw:" IPSTR,
					IP2STR(&evt->event_info.got_ip.ip),
					IP2STR(&evt->event_info.got_ip.mask),
					IP2STR(&evt->event_info.got_ip.gw));
			printf("\n");


			sprintf(ip,IPSTR,IP2STR(&evt->event_info.got_ip.ip));

			break;
		case EVENT_SOFTAPMODE_STACONNECTED:
			printf("station: " MACSTR "join, AID = %d\n",
					MAC2STR(evt->event_info.sta_connected.mac),
					evt->event_info.sta_connected.aid);
			break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
			printf("station: " MACSTR "leave, AID = %d\n",
					MAC2STR(evt->event_info.sta_disconnected.mac),
					evt->event_info.sta_disconnected.aid);
			break;
		default:
			break;
	}
}


bool ala_set_firmware(char* value)
{
	return true;
}

char *ala_get_ip()
{
	return ip;
}

bool ala_set_ip(char* value)
{
	return true;
}

char* ala_get_firmware()
{
	sprintf(firmware_value,"%s","led_light_02:1");
	return firmware_value;
}

bool ala_set_state(char* value)
{
	return true;
}

char* ala_get_state()
{
	sprintf(state_value,"%s","state is on");
	return state_value;
}

char* ala_get_rgb()
{
	sprintf(rgb_value,"%d#%d#%d",red,green,blue);
	return rgb_value;
}

bool ala_set_rgb(char* value)
{
	printf("ala_set_rgb %s\n", value);

	char *color=zalloc(5);
	uint8 i=0;
	uint8 len=strlen(value);
	uint8 colori=0;
	uint8 chi=0;
	for(i=0;i<len;i++)
	{
		if(value[i]!='#')
		{
			color[chi]=value[i];
			chi++;
		}
		else
		{
			color[chi]=0;
			chi=0;
			if(colori==0)
			{
				red=atoi(color);
			}
			else if(colori==1)
			{
				green=atoi(color);
			}
			colori++;

		}
	}
	color[chi]=0;
	blue=atoi(color);
	free(color);


	printf("rgb:%d,%d,%d,%d,%d", red,green,blue,rgblevel,((red+1)*rgblevel-1)>0?((red+1)*rgblevel-1):0);
	if(on==1)
	{
		aipwm_set(0, red*rgblevel);
		aipwm_set(1, green*rgblevel);
		aipwm_set(2, blue*rgblevel);
	}

	uint8 allvalue[16];
	allvalue[0]=0xEE;
	allvalue[1]=red&0x00FF;
	allvalue[2]=red>>8;
	allvalue[3]=green&0x00FF;
	allvalue[4]=green>>8;
	allvalue[5]=blue&0x00FF;
	allvalue[6]=blue>>8;
	allvalue[7]=rgblevel&0x00FF;
	allvalue[8]=rgblevel>>8;
	allvalue[9]=wlevel&0x00FF;
	allvalue[10]=wlevel>>8;
	allvalue[11]=on;
	allvalue[12]=deepsleep;

	spi_flash_erase_sector(0x55);
	spi_flash_write(0x55*4096,(uint32*)allvalue, 16);


	printf("rgb:ok\n");
	return true;
}


char* ala_get_rgblevel()
{
	sprintf(rgblevel_value,"%d",rgblevel);
	return rgblevel_value;
}

bool ala_set_rgblevel(char* value)
{

	rgblevel=atoi(value);

	printf("ala_set_rgblevel %d\n",rgblevel);

	if(on==1)
	{
		aipwm_set(0, red*rgblevel);
		aipwm_set(1, green*rgblevel);
		aipwm_set(2, blue*rgblevel);
	}
	uint8 allvalue[16];
	allvalue[0]=0xEE;
	allvalue[1]=red&0x00FF;
	allvalue[2]=red>>8;
	allvalue[3]=green&0x00FF;
	allvalue[4]=green>>8;
	allvalue[5]=blue&0x00FF;
	allvalue[6]=blue>>8;
	allvalue[7]=rgblevel&0x00FF;
	allvalue[8]=rgblevel>>8;
	allvalue[9]=wlevel&0x00FF;
	allvalue[10]=wlevel>>8;
	allvalue[11]=on;
	allvalue[12]=deepsleep;

	spi_flash_erase_sector(0x55);
	spi_flash_write(0x55*4096,(uint32*)allvalue, 16);


	return true;
}

char* ala_get_wlevel()
{
	sprintf(wlevel_value,"%d",wlevel);
	return wlevel_value;
}

bool ala_set_wlevel(char* value)
{

	wlevel=atoi(value);

	printf("ala_set_wlevel %d\n",wlevel);

	if(on==1)
	{
		aipwm_set(3, (256*wlevel-1)>0?(256*wlevel-1):0);
	}

	uint8 allvalue[16];
	allvalue[0]=0xEE;
	allvalue[1]=red&0x00FF;
	allvalue[2]=red>>8;
	allvalue[3]=green&0x00FF;
	allvalue[4]=green>>8;
	allvalue[5]=blue&0x00FF;
	allvalue[6]=blue>>8;
	allvalue[7]=rgblevel&0x00FF;
	allvalue[8]=rgblevel>>8;
	allvalue[9]=wlevel&0x00FF;
	allvalue[10]=wlevel>>8;
	allvalue[11]=on;
	allvalue[12]=deepsleep;

	spi_flash_erase_sector(0x55);
	spi_flash_write(0x55*4096,(uint32*)allvalue, 16);


	return true;
}

char* ala_get_on()
{
	if(on==1)
		sprintf(on_value,"%s","1");
	else
		sprintf(on_value,"%s","0");

	return on_value;
}

bool ala_set_on(char* value)
{
	on=atoi(value);
	if(on==1)
	{
		printf("ala_set_on\n");

		aipwm_set(0, red*rgblevel);
		aipwm_set(1, green*rgblevel);
		aipwm_set(2, blue*rgblevel);
		aipwm_set(3, (256*wlevel-1)>0?(256*wlevel-1):0);

	}
	else
	{
		printf("ala_set_off\n");
		aipwm_set(0, 0);
		aipwm_set(1, 0);
		aipwm_set(2, 0);
		aipwm_set(3, 0);

	}

	uint8 allvalue[16];
	allvalue[0]=0xEE;
	allvalue[1]=red&0x00FF;
	allvalue[2]=red>>8;
	allvalue[3]=green&0x00FF;
	allvalue[4]=green>>8;
	allvalue[5]=blue&0x00FF;
	allvalue[6]=blue>>8;
	allvalue[7]=rgblevel&0x00FF;
	allvalue[8]=rgblevel>>8;
	allvalue[9]=wlevel&0x00FF;
	allvalue[10]=wlevel>>8;
	allvalue[11]=on;
	allvalue[12]=deepsleep;

	spi_flash_erase_sector(0x55);
	spi_flash_write(0x55*4096,(uint32*)allvalue, 16);

	return true;
}


bool ala_set_blinkval(char *value)
{
	strcpy(blink_value,value);
	char *color=zalloc(5);
	uint8 i=0;
	uint8 len=strlen(value);
	uint8 colori=1;
	uint8 chi=0;
	printf("color=%s\n",value);
	for(i=0;i<len;i++)
	{
		if(value[i]!='#')
		{
			color[chi]=value[i];
			chi++;
		}
		else
		{
			color[chi]=0;
			chi=0;
			blink_vals[colori]=atoi(color);
			colori++;
		}
	}

	color[chi]=0;
	blink_vals[colori]=atoi(color);
	printf("color=%s\n",color);
	blink_vals[0]=colori+1;
	free(color);

	return true;
}

char* ala_get_blinkval()
{
	return blink_value;
}

void task_blink(void *pvParameters)
{
	int count=blink_vals[0]-1;
	count=count/3;
	uint8 ired=0;
	uint8 igreen=0;
	uint8 iblue=0;
	uint8 i=0;
	while(true)
	{
		for(i=0;i<count;i++)
		{
			ired=blink_vals[i*3+1];
			igreen=blink_vals[i*3+2];
			iblue=blink_vals[i*3+3];
			if(on==1)
			{
				aipwm_set(0, ired*rgblevel);
				aipwm_set(1, igreen*rgblevel);
				aipwm_set(2, iblue*rgblevel);
				printf("rgb=%d,%d,%d\r\n",ired,igreen,iblue);
			}
			vTaskDelay(blink_time / portTICK_RATE_MS);
		}
	}

}
bool ala_set_blinktime(char *value)
{
	blink_time=atoi(value);
	if(blink_time>0&&xHandleBlink==NULL)
	{
		xTaskCreate(task_blink, "tsk_blink", 256, NULL, 1, &xHandleBlink);
	}
	else
	{
		if(xHandleBlink!=NULL)
		{
			vTaskDelete(xHandleBlink);
			xHandleBlink=NULL;
		}
	}
	return true;
}

char* ala_get_blinktime()
{
	return "";
}








void uart_rx_intr_enable(uint8 uart_no)
{
#if 1
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
#else
    ETS_UART_INTR_ENABLE();
#endif
}

LOCAL void uart0_rx_intr_handler(void *para)
{
	uint8 fifo_len = (READ_PERI_REG(UART_STATUS(0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
	uint8 d_tmp = 0;
	uint8 idx=0;
	for(idx=0;idx<fifo_len;idx++) {
		d_tmp = READ_PERI_REG(UART_FIFO(0)) & 0xFF;
		if(d_tmp=='R')
		{
			aipwm_set(0, 3000);
			aipwm_set(1, 0);
			aipwm_set(2, 0);
			aipwm_set(3, 0);
			os_printf("red ok\n");
		}
		else if(d_tmp=='G')
		{
			aipwm_set(0, 0);
			aipwm_set(1, 3000);
			aipwm_set(2, 0);
			aipwm_set(3, 0);
			os_printf("green ok\n");
		}
		else if(d_tmp=='B')
		{
			aipwm_set(0, 0);
			aipwm_set(1, 0);
			aipwm_set(2, 3000);
			aipwm_set(3, 0);
			os_printf("blue ok\n");
		}

	}
	WRITE_PERI_REG(UART_INT_CLR(0), UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
	uart_rx_intr_enable(0);
}

void timer1()
{
	ala_timer();
}

uint16 ledloop=0;

void test_timer2(void)
{

	if(ledloop<10)
	{
		ledloop++;
		if(ledloop==5)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(1),0);

		}
	}

	if(GPIO_INPUT_GET(GPIO_ID_PIN(12))==1)
	{

	}
	else
	{
		if(on==1)
		{
			ala_set_on("0");
		}
		else
		{
			ala_set_on("1");
		}
	}
}
/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{




	UART_WaitTxFifoEmpty(UART0);
	UART_ConfigTypeDef uart;
	uart.baud_rate=BIT_RATE_115200;
	uart.data_bits=UART_WordLength_8b;
	uart.parity=USART_Parity_None;
	uart.stop_bits=USART_StopBits_1;
	uart.flow_ctrl=USART_HardwareFlowControl_None;
	uart.UART_RxFlowThresh=120;
	uart.UART_InverseMask=UART_None_Inverse;
	UART_ParamConfig(UART0,&uart);
	UART_SetPrintPort(UART0);
	UART_intr_handler_register(uart0_rx_intr_handler,0);
	ETS_UART_INTR_ENABLE();


	uint8_t value[16];
	spi_flash_read(0x55*4096,(uint32*)value, 16);
	aipwm_init();
	deepsleep=value[12];

	if(value[0]==0xEE)
	{
		red=value[2]*256+value[1];
		green=value[4]*256+value[3];
		blue=value[6]*256+value[5];
		rgblevel=value[8]*256+value[7];
		wlevel=value[10]*256+value[9];
		on=value[11];

		if(on==1)
		{
			aipwm_set(0, red*rgblevel);
			aipwm_set(1, green*rgblevel);
			aipwm_set(2, blue*rgblevel);
			aipwm_set(3, (256*wlevel-1)>0?(256*wlevel-1):0);

		}
		else
		{
			aipwm_set(0, 0);
			aipwm_set(1, 0);
			aipwm_set(2, 0);
			aipwm_set(3, 0);
		}


	}
	else
	{
		red=0;
		green=0;
		blue=0;
		rgblevel=16;
		wlevel=16;
		on=1;

		aipwm_set(0, red*rgblevel);
		aipwm_set(1, green*rgblevel);
		aipwm_set(2, blue*rgblevel);
		aipwm_set(3, (256*wlevel-1)>0?(256*wlevel-1):0);

	}
/*

	if(deepsleep==0)
	{
		deepsleep=1;
		printf("deepsleep=1\n");
	}
	else
	{
		deepsleep=0;
		printf("deepsleep=0\n");
	}
*/

	value[0]=0xEE;
	value[1]=red&0x00FF;
	value[2]=red>>8;
	value[3]=green&0x00FF;
	value[4]=green>>8;
	value[5]=blue&0x00FF;
	value[6]=blue>>8;
	value[7]=rgblevel&0x00FF;
	value[8]=rgblevel>>8;
	value[9]=wlevel&0x00FF;
	value[10]=wlevel>>8;
	value[11]=on;
	value[12]=deepsleep;

	spi_flash_erase_sector(0x55);
	spi_flash_write(0x55*4096,(uint32*)value, 16);



/*
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
	GPIO_AS_OUTPUT(1);//LED
	GPIO_OUTPUT_SET(GPIO_ID_PIN(1),1);
*/

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(12));
	GPIO_AS_INPUT(12);


	os_timer_disarm(&mytimer);
	os_timer_setfn(&mytimer,(ETSTimerFunc *)timer1,NULL);
	os_timer_arm(&mytimer,1000,1);
/*
	if(deepsleep==1)
	{
		printf("deepsleep.................");
		//GPIO_OUTPUT_SET(GPIO_ID_PIN(1),1);
		aipwm_set(0, 0);
		aipwm_set(1, 0);
		aipwm_set(2, 0);
		aipwm_set(3, 0);
		system_deep_sleep(0);

	}
*/
	wifi_set_opmode(STATION_MODE);

	struct station_config *config = (struct station_config *)zalloc(sizeof(struct station_config));
	wifi_station_get_config(config);

	sprintf(config->ssid,"CMCC-EDU");
	sprintf(config->password,"Freescale503");

	wifi_station_set_config(config);
	free(config);
	bzero(&server_addr, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(1234);


	if(strcmp(config->ssid,"")==0)
	{
		xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL, 2, NULL);
	}


	strcpy(phoneuuid,"none");

	ala_init("www.trencon.com",9999,get_deviceid(),get_devicetype(),version,ip,wifiid,wifipwd,phoneuuid);
	ala_attach_attribute("rgb",ala_set_rgb,ala_get_rgb);
	ala_attach_attribute("rgblevel",ala_set_rgblevel,ala_get_rgblevel);
	ala_attach_attribute("wlevel",ala_set_wlevel,ala_get_wlevel);
	ala_attach_attribute("on",ala_set_on,ala_get_on);
	ala_attach_attribute("state",ala_set_state,ala_get_state);
	ala_attach_attribute("ip",ala_set_ip,ala_get_ip);
	ala_attach_attribute("blinkval",ala_set_blinkval,ala_get_blinkval);
	ala_attach_attribute("blinktime",ala_set_blinktime,ala_get_blinktime);
	ala_attach_attribute("firmware",ala_set_firmware,ala_get_firmware);

	wifi_set_event_handler_cb(wifi_handle_event_cb);


	xTaskCreate(task_connect, "tsk_connect", 256, NULL, 1, &xHandleConnect);



	hw_timer_init(1);
	hw_timer_arm(500000);
	hw_timer_set_func(test_timer2);



}

