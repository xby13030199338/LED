/*
 * alacloud.c
 *
 *  Created on: 2016年5月27日
 *      Author: Administrator
 */

#include "alasmart/alacloud.h"
#include "ssl/ssl_bigint.h"
#include "hw_timer.h"
#define delay_us   os_delay_us  //系统微妙级

char attribute_delayset[20][16];
char attribute_name[20][20];
void *attribute_setter[20];
uint32 attribute_delay[20];
void *attribute_getter[20];
char receive_name[20][20];
char receive_value[20][20];
char send_name[20][20];
char send_value[20][20];
char array_temp[50];

char *mycatalog;
char *myuserid;
char *mydeviceid;
char *mydevicetype;
char *myip;
char *mywifiid;
char *mywifipwd;
char *myphoneuuid;

int32 nowHour=-1;
int32 nowMinute=-1;
int32 nowSecond=-1;
int32 openHour=-1;
int32 openMinute=-1;
int32 closeHour=-1;
int32 closeMinute=-1;
uint32 flag[1];
uint32 delay_flag[1];
uint8 mydeviceversion;
uint8 idx=0;
uint8_t src_out[200];
uint8_t des_out[200];
char getret[50];
char array[20][20];

AES_CTX aes_ctx;

uint8_t iv[16]="0102030405060708";
uint8_t *key;

uint8 start_count=0;

char opentimer_value[20];
char closetimer_value[20];


typedef void ETSTimerFunc(void *timer_arg);
os_timer_t atimer;


char *get_opentimer()
{
	sprintf(opentimer_value,"%d#%d",openHour,openMinute);
	return opentimer_value;
}

bool set_opentimer(char* value)
{
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
				openHour=atoi(color);
			}
			else if(colori==1)
			{
				openMinute=atoi(color);
			}
			else if(colori==2)
			{
				nowHour=atoi(color);
			}
			else if(colori==3)
			{
				nowMinute=atoi(color);
			}
			colori++;
		}
	}
	color[chi]=0;
	nowSecond=atoi(color);
	free(color);

	flag[0] = 0x99999999;
	system_rtc_mem_write(65,flag,4);
	system_rtc_mem_write(66,&nowHour,4);
	system_rtc_mem_write(67,&nowMinute,4);
	system_rtc_mem_write(68,&nowSecond,4);
	system_rtc_mem_write(69,&openHour,4);
	system_rtc_mem_write(70,&openMinute,4);
	system_rtc_mem_write(71,&closeHour,4);
	system_rtc_mem_write(72,&closeMinute,4);

	return true;
}


char *get_closetimer()
{
	sprintf(closetimer_value,"%d#%d",closeHour,closeMinute);
	return closetimer_value;
}

bool set_closetimer(char* value)
{
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
				closeHour=atoi(color);
			}
			else if(colori==1)
			{
				closeMinute=atoi(color);
			}
			else if(colori==2)
			{
				nowHour=atoi(color);
			}
			else if(colori==3)
			{
				nowMinute=atoi(color);
			}

			colori++;
		}
	}
	color[chi]=0;
	nowSecond=atoi(color);
	free(color);

	flag[0] = 0x99999999;
	system_rtc_mem_write(65,flag,4);
	system_rtc_mem_write(66,&nowHour,4);
	system_rtc_mem_write(67,&nowMinute,4);
	system_rtc_mem_write(68,&nowSecond,4);
	system_rtc_mem_write(69,&openHour,4);
	system_rtc_mem_write(70,&openMinute,4);
	system_rtc_mem_write(71,&closeHour,4);
	system_rtc_mem_write(72,&closeMinute,4);



	return true;
}

double myatof(const char* sptr)
{
    double temp=10;
    bool ispnum=true;
    double ans=0;
    if(*sptr=='-')
    {
        ispnum=false;
        sptr++;
    }
    else if(*sptr=='+')
    {
        sptr++;
    }

    while(*sptr!='\0')
    {
        if(*sptr=='.'){ sptr++;break;}
        ans=ans*10+(*sptr-'0');
        sptr++;
    }
    while(*sptr!='\0')
    {
        ans=ans+(*sptr-'0')/temp;
        temp*=10;
        sptr++;
    }
    if(ispnum) return ans;
    else return ans*(-1);
}


void ala_attach_attribute(char* name,bool (*ptr1)(char*),char* (*ptr2)(void))
{
	sprintf(attribute_name[idx], "%s", name);
	attribute_setter[idx]=*ptr1;
	attribute_getter[idx]=*ptr2;


	idx++;
}

int ala_parse_receive(uint8 local,char* deviceid,char type,int size,char *value)
{
	int parse_ret=0;
	printf("type=%d\n",type);
	printf("deviceid=%s\n",deviceid);

	if(type==1&&local==1)//设置用户名和密码
	{
		spi_flash_erase_sector(0x52);
		uint32 writelen=size;
		uint8 header[4]={0xEF,0xEF,0xEF,0xEF};
		spi_flash_write(0x52*4*1024,(uint32*)header, 4);
		spi_flash_write(0x52*4*1024+4,&writelen, 4);
		spi_flash_write(0x52*4*1024+8,(uint32*)value, size);
		uint32 c=0x01010101;
		spi_flash_write(0x52*4*1024+190,&c, 4);

		char delims[] = ",";
		char *result = NULL;
		value[size]=0;
		char* userid= strtok( value, delims );
		char* userpwd= strtok( NULL, delims );
		char* catalog= strtok( NULL, delims );
		printf("用户名=%s\n",userid);
		printf("密码=%s*\n",userpwd);
		printf("分类=%s*\n",catalog);
		memcpy(myuserid,userid,32);
		memcpy(key,userpwd,16);
		strcpy(mycatalog,catalog);


		uint16 len=strlen(mydeviceid);
		uint8_t i=0;
		for(i=0;i<len;i++)
		{
			value[i]=mydeviceid[i];
		}
		value[len]='\n';
		parse_ret=len+1;


	}

	if(type==0xBB)
	{
		//printf("设备ip=%s,%s\n",myip,deviceid);
		char *temp=zalloc(100);
		if(strcmp(deviceid,mydeviceid)==0)
		{
			temp=strcat(temp,myip);
		}
		else
		{
			temp=strcat(temp,"1.1.1.1");
		}

		temp=strcat(temp,",");
		temp=strcat(temp,mydeviceid);

		uint16 useridlen=strlen(temp);
		uint16 toallen=4+useridlen+1;
		uint8 *retbuf=zalloc(12+useridlen+1);

		retbuf[0]=0xEF;
		retbuf[1]=0xEF;
		retbuf[2]=0xEF;
		retbuf[3]=0xEF;
		retbuf[4]=0xEF;
		retbuf[5]=0xBB;
		retbuf[6]=toallen&0x00ff;
		retbuf[7]=toallen>>8;
		retbuf[8]=useridlen&0x00ff;
		retbuf[9]=useridlen>>8;
		retbuf[10]=0x01;
		retbuf[11]=0x00;
		uint8 i=0;
		for(i=0;i<useridlen;i++)
		{
			retbuf[12+i]=temp[i];
		}
		retbuf[12+useridlen+i]=0xBB;

		memcpy(value,retbuf,12+useridlen+1);
		parse_ret=12+useridlen+1;
		free(retbuf);
		free(temp);

	}
	else if(type==0x30)
	{

		uint8 len=size;

		uint8_t *out=zalloc(len);
		uint8_t i=0;
		for(i=0;i<len;i++)
		{
			out[i]=value[i];
		}

		AES_set_key(&aes_ctx,key,iv,AES_MODE_128);
		AES_convert_key(&aes_ctx);

		uint8_t *txtout=zalloc(len+1);
		AES_cbc_decrypt(&aes_ctx, out, txtout, len);
		txtout[len]=0;

		char delims[] = ",";


		char* op= strtok( txtout, delims );

		if(strcmp(op,"update")==0)
		{
			spi_flash_erase_sector(0x56);
			uint8 header[4]={0xCC,0xCC,0xCC,0xCC};
			spi_flash_write(0x56*4*1024,(uint32*)header, 4);
			parse_ret=-1;

		}
		else
		{
			free(out);
			free(txtout);
			parse_ret=0;
		}
	}
	else if(type==0x02)
	{

		uint8 i=0;
		uint8 j=0;


		for(i=0;i<size;i++)
		{
			src_out[i]=value[i];
		}
		AES_set_key(&aes_ctx,key,iv,AES_MODE_128);
		AES_convert_key(&aes_ctx);
		AES_cbc_decrypt(&aes_ctx, src_out, des_out, size);
		des_out[size]=0;

		//printf("decrypt is=%s \r\n",des_out);


		uint8 chi=0;
		uint8 arrayi=0;
		uint8 len=strlen(des_out);
		for(i=0;i<len;i++)
		{
			if(des_out[i]!=',')
			{
				array_temp[chi]=des_out[i];
				chi++;
			}
			else
			{
				array_temp[chi]=0;
				chi=0;
				strcpy(array[arrayi],array_temp);
				arrayi++;
			}
		}


		char* s_userid=array[0];
		char* s_devid= array[1];
		char* s_info1=array[2];
		char* s_info2=array[3];
		char* s_info3=array[4];
		char* optype = array[5];
		char* opcount = array[6];

		if(strcmp(optype,"set")==0)
		{
			value[0]=0;
			value=strcat(value,deviceid);
			value=strcat(value,"set,");

			value=strcat(value,opcount);
			value=strcat(value,",");

			//printf("attri_count is=%s \r\n",result);
			uint8 attri_count=atoi(opcount);



			for(i=0;i<attri_count;i++)
			{
				char *s_delay = array[7+i*3];
				char *s_name =  array[7+i*3+1];
				char *s_value =  array[7+i*3+2];


				value=strcat(value,s_delay);
				value=strcat(value,",");
				value=strcat(value,s_name);

				value=strcat(value,",");

				bool find=false;

				if(strcmp("opentimer",s_name)==0)
				{
					set_opentimer(s_value);
					value=strcat(value,get_opentimer());
					value=strcat(value,",");
					find=true;
				}

				if(strcmp("closetimer",s_name)==0)
				{
					set_closetimer(s_value);
					value=strcat(value,get_closetimer());
					value=strcat(value,",");
					find=true;
				}

				for(j=0;j<20;j++)
				{


					if(strcmp(attribute_name[j],s_name)==0)
					{
						attribute_delay[j]=atoi(s_delay);
						strcpy(attribute_delayset[j],s_value);

						//printf("delay setter %s,%s,%s",s_delay,s_name,s_value);

						if(attribute_delay[j]==0)
						{
							//printf("now setter %s,%s,%s",s_delay,s_name,s_value);

							ala_callback_setter setter=(ala_callback_setter)(attribute_setter[j]);
							ala_callback_getter getter=(ala_callback_getter)(attribute_getter[j]);
							(*setter)(attribute_delayset[j]);
							value=strcat(value,(*getter)());
							value=strcat(value,",");

						}
						else
						{
							value=strcat(value,attribute_delayset[j]);
							value=strcat(value,",");
						}
						find=true;
						break;
					}

				}
				if(!find){
					value=strcat(value,"error,");
				}


			}

			if(strcmp(s_userid,s_devid)==0)
			{
				parse_ret=0;
			}
			else
			{

				///////////////////////

				uint16 len=strlen(value);
				uint8 a=len/16;
				uint8 b=len%16;
				uint8 c=16-b;

				if(b>0){
					a++;
				}
				for(i=0;i<c;i++){
					value[len+i]='A';
				}
				value[a*16]=0;

				//printf("retvalue is=%s \r\n",value);

				AES_set_key(&aes_ctx,key,iv,AES_MODE_128);
				AES_cbc_encrypt(&aes_ctx,value,des_out,a*16);

				uint16 useridlen=strlen(s_userid);
				uint16 toallen=4+useridlen+a*16;

				value[0]=0xEF;
				value[1]=0xEF;
				value[2]=0xEF;
				value[3]=0xEF;
				value[4]=0xEF;
				value[5]=0x12;
				value[6]=toallen&0x00ff;
				value[7]=toallen>>8;
				value[8]=useridlen&0x00ff;
				value[9]=useridlen>>8;
				value[10]=(a*16)&0x00ff;
				value[11]=(a*16)>>8;
				for(i=0;i<useridlen;i++)
				{
					value[12+i]=s_userid[i];
				}
				for(i=0;i<a*16;i++)
				{
					value[12+useridlen+i]=des_out[i];
				}

				parse_ret=8+toallen;

			}


		}
		else if(strcmp(optype,"get")==0)
		{
			value[0]=0;
			value=strcat(value,deviceid);
			value=strcat(value,"get,");

			value=strcat(value,opcount);
			value=strcat(value,",");


			uint8 attri_count=atoi(opcount);


			//printf("attri_count is=%d \r\n",attri_count);

			for(i=0;i<attri_count;i++)
			{
				strcpy(receive_name[i],array[7+i*2]);
				strcpy(receive_value[i],array[7+i*2+1]);

			}
			for(i=0;i<attri_count;i++)
			{
				strcpy(send_value[i],"error");


				if(strcmp("opentimer",receive_name[i])==0)
				{

					strcpy(send_value[i],get_opentimer());
					strcpy(send_name[i],receive_name[i]);


				}

				if(strcmp("closetimer",receive_name[i])==0)
				{
					strcpy(send_value[i],get_closetimer());
					strcpy(send_name[i],receive_name[i]);
				}

				for(j=0;j<20;j++)
				{


					if(strcmp(attribute_name[j],receive_name[i])==0)
					{
						//printf("attribute_name is=%s \r\n",attribute_name[j]);
						ala_callback_getter getter=(ala_callback_getter)attribute_getter[j];

						strcpy(send_value[i],(*getter)());
						strcpy(send_name[i],receive_name[i]);


					}
				}
			}

			if(strcmp(s_userid,s_devid)==0)
			{
				parse_ret=0;
			}
			else
			{


				for(i=0;i<attri_count;i++)
				{
					sprintf(src_out,"%s,%s,",send_name[i],send_value[i]);
					value=strcat(value,src_out);
				}

				uint16 len=strlen(value);
				uint8 a=len/16;
				uint8 b=len%16;
				uint8 c=16-b;

				if(b>0){
					a++;
				}
				for(i=0;i<c;i++){
					value[len+i]='A';
				}
				value[a*16]=0;

				//printf("retvalue is=%s \r\n",value);


				AES_set_key(&aes_ctx,key,iv,AES_MODE_128);

				AES_cbc_encrypt(&aes_ctx,value,des_out,a*16);

				uint16 useridlen=strlen(s_userid);
				uint16 toallen=4+useridlen+a*16;

				value[0]=0xEF;
				value[1]=0xEF;
				value[2]=0xEF;
				value[3]=0xEF;
				value[4]=0xEF;
				value[5]=0x12;
				value[6]=toallen&0x00ff;
				value[7]=toallen>>8;
				value[8]=useridlen&0x00ff;
				value[9]=useridlen>>8;
				value[10]=(a*16)&0x00ff;
				value[11]=(a*16)>>8;
				for(i=0;i<useridlen;i++)
				{
					value[12+i]=s_userid[i];
				}
				for(i=0;i<a*16;i++)
				{
					value[12+useridlen+i]=des_out[i];
				}

				parse_ret=8+toallen;


			}
		}
		else if(strcmp(optype,"del")==0)
		{
			struct station_config *config = (struct station_config *)zalloc(sizeof(struct station_config));
			wifi_station_get_config(config);
			sprintf(config->ssid,"");
			sprintf(config->password,"");
			wifi_station_set_config(config);
			free(config);

			spi_flash_erase_sector(0x55);
			uint8 header[4]={0x00,0x00,0x00,0x00};
			spi_flash_write(0x55*4096,(uint32*)header, 4);

			parse_ret=0;
		}




	}

	return parse_ret;

}

short ala_connect_platform(char* ret)
{

	short len1=strlen(mydeviceid);
	short len2=strlen(mydevicetype);
	short len3=strlen(myip);
	short len4=strlen(myphoneuuid);
	short len=len1+len2+len3+len4+3;

	len=len+1;
	ret[0]=0xEF;
	ret[1]=0xEF;
	ret[2]=0xEF;
	ret[3]=0xEF;
	ret[4]=0xEF;
	ret[5]=0xAA;
	ret[6]=(char)len;
	ret[7]=(char)(len>>8);

	unsigned char i=0;
	for(i=0;i<len1;i++){
		ret[i+8]=mydeviceid[i];
	}
	ret[len1+8]=',';
	for(i=0;i<len2;i++){
		ret[i+len1+9]=mydevicetype[i];
	}

	ret[len1+len2+9]=',';

	for(i=0;i<len3;i++){
		ret[i+len1+len2+10]=myip[i];
	}

	ret[len1+len2+len3+10]=',';

	for(i=0;i<len4;i++){
		ret[i+len1+len2+len3+11]=myphoneuuid[i];
	}
	ret[len1+len2+len3+len4+11]=0;
	printf("platform:%s",myphoneuuid);
	strcpy(myphoneuuid,"none");
	return len+8;
}

void ala_timer()
{

	if(nowHour!=-1&&nowMinute!=-1)
	{
		nowSecond++;
		if(nowSecond==60)
		{
			nowSecond=0;
			nowMinute++;
			if(nowMinute==60)
			{
				nowMinute=0;
				nowHour++;
				if(nowHour==24)
				{
					nowHour=0;
				}
			}
		}
		system_rtc_mem_write(66,&nowHour,4);
		system_rtc_mem_write(67,&nowMinute,4);
		system_rtc_mem_write(68,&nowSecond,4);


		uint8 j=0;
		if(nowHour==openHour&&nowMinute==openMinute&&nowSecond==0)
		{
			for(j=0;j<20;j++)
			{
				if(strcmp(attribute_name[j],"on")==0)
				{
					ala_callback_setter setter=(ala_callback_setter)(attribute_setter[j]);
					(*setter)("1");
					break;
				}
			}
		}

		if(nowHour==closeHour&&nowMinute==closeMinute&&nowSecond==0)
		{
			for(j=0;j<20;j++)
			{
				if(strcmp(attribute_name[j],"on")==0)
				{
					ala_callback_setter setter=(ala_callback_setter)(attribute_setter[j]);
					(*setter)("0");
					break;
				}
			}
		}

	}

	if(start_count<10)
	{
		start_count++;
	}
	else if(start_count==10)
	{
		uint8_t buf[4];
		spi_flash_read(0x51*4*1024,(uint32*)buf, 4);

		buf[0]=0x01;
		buf[1]=0x00;
		buf[2]=0x00;
		buf[3]=0x00;

		spi_flash_erase_sector(0x51);
		spi_flash_write(0x51*4*1024,(uint32*)buf, 4);


		start_count++;


	}



	uint8 i;
	for(i=0;i<20;i++)
	{
		uint32 delay=attribute_delay[i];

		if(delay>0)
		{

			delay--;
			attribute_delay[i]=delay;

			if(delay==0)
			{
				ala_callback_setter setter=(ala_callback_setter)attribute_setter[i];

				(*setter)(attribute_delayset[i]);

			}

		}



	}

}



void ala_init(char* server,uint16 port,char* deviceid,char* devicetype,uint8 version,char *ip,char *wifiid,char *wifipwd,char *phoneuuid)
{
	mydeviceversion=version;
	mydeviceid=deviceid;
	mydevicetype=devicetype;
	myip=ip;
	mywifiid=wifiid;
	mywifipwd=wifipwd;
	myphoneuuid=phoneuuid;

	key=zalloc(16);
	myuserid=zalloc(32);
	mycatalog=zalloc(50);
	uint8 i=0;




	uint8_t bufnum[4];
	spi_flash_read(0x51*4*1024,(uint32*)bufnum, 4);

	uint8 c=bufnum[0];
	bufnum[0]=c+1;
	if(bufnum[0]>=6)
	{
		bufnum[0]=0x01;
		bufnum[1]=0x00;
		bufnum[2]=0x00;
		bufnum[3]=0x00;

		printf("恢复出厂设置\n");
		struct station_config *config = (struct station_config *)zalloc(sizeof(struct station_config));
		wifi_station_get_config(config);
		sprintf(config->ssid,"");
		sprintf(config->password,"");
		wifi_station_set_config(config);
		free(config);

		spi_flash_erase_sector(0x55);
		uint8 header[4]={0x00,0x00,0x00,0x00};
		spi_flash_write(0x55*4096,(uint32*)header, 4);
	}
	spi_flash_erase_sector(0x51);
	spi_flash_write(0x51*4*1024,(uint32*)bufnum, 4);


	uint8_t buf[200];
	spi_flash_read(0x52*4*1024,(uint32*)buf, 200);
	if(buf[0]==0xEF&&buf[1]==0xEF&&buf[2]==0xEF&&buf[3]==0xEF)
	{
		uint16 len=buf[4]+buf[5]*256;
		for(i=0;i<192;i++)
		{
			buf[i]=buf[i+8];
		}
		buf[len]=0;
		char delims[] = ",";
		char *result = NULL;
		char* userid= strtok( buf, delims );
		char* userpwd= strtok( NULL, delims );
		char* catalog= strtok( NULL, delims );
		memcpy(myuserid,userid,32);
		memcpy(key,userpwd,16);
		if(catalog!=NULL)
		{
			strcpy(mycatalog,catalog);
		}

	}


	uint32 istimer[1];
	system_rtc_mem_read(65, istimer, 4);

	if(istimer[0]==0x99999999)
	{
		system_rtc_mem_read(66, &nowHour, 4);
		system_rtc_mem_read(67, &nowMinute, 4);
		system_rtc_mem_read(68, &nowSecond, 4);
		system_rtc_mem_read(69, &openHour, 4);
		system_rtc_mem_read(70, &openMinute, 4);
		system_rtc_mem_read(71, &closeHour, 4);
		system_rtc_mem_read(72, &closeMinute, 4);

	}
	os_printf("Trencon Technology Co., Ltd. Ala Smart Light ready... \r\n\r\n\r\n");
}

void ala_pwd(char *pwd)
{
	memcpy(key,pwd,16);

}

uint32 ala_get_delay(char* value)
{
	uint8 i=0;
	for(i=0;i<20;i++)
	{
		if(strcpy(attribute_name[i],value)==0)
		{
			return attribute_delay[i];
		}
	}
	return 0;
}



void ala_free()
{

	printf("bye alacloud \n");
}
