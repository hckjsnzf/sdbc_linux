#include <sdbc.h>
#include <json_pack.h>

extern JSON_OBJECT dm_app(GDA *gp,JSON_OBJECT json,JSON_OBJECT err_json);

/******************************************************
 * ��̬ģ���Ӧ�÷������,svc��
 * head->data="{model:"modelname",param:{....}}"
 *              model��ģ����
 *              param���ύ��ģ�������
 ******************************************************/
int dmapp(T_Connect *conn,T_NetHead *head)
{
T_SRV_Var *srvp=(T_SRV_Var *)conn->Var;
GDA *gp=(GDA *)srvp->var;
JSON_OBJECT json,err_json,result=NULL;
char msg[2048];
int ret,event=head->PROTO_NUM;
int cont=head->ERRNO2;

	conn->status=(head->ERRNO2==PACK_STATUS)?1:0;
	
	err_json=json_object_new_array();
	json=json_tokener_parse(head->data);
	if(!json) {
		sprintf(msg,"�����JSON ��ʽ��,PKG_LEN=%d",head->PKG_LEN);
                json_object_array_add(err_json,jerr(100,msg));
		head->ERRNO1=FORMATERR;
		head->ERRNO2=conn->status>0?PACK_NOANSER:-1;
		return_error(conn,head,json_object_to_json_string(err_json));
		conn->status=0;
		ShowLog(1,"%s:%s",__FUNCTION__,json_object_to_json_string(err_json));
		json_object_put(err_json);
		return 0;
	}

	result=dm_app(gp,json,err_json); //����Ӧ��ģ��

	ret=0;
	if(result == (JSON_OBJECT)-1) ret=THREAD_ESCAPE;	
	else if(!result) {
		head->data=(char *)json_object_to_json_string(err_json);
		head->ERRNO1=-1;
		head->ERRNO2=conn->status>0?PACK_NOANSER:-1;
	} else {
		json_object_object_add(result,"status",err_json);
		head->data=(char *)json_object_to_json_string(result);
		head->ERRNO1=0;
		head->ERRNO2=conn->status>0?PACK_STATUS:0; 
	}

	if(ret==0 && cont != PACK_NOANSER) {
		head->PKG_LEN=strlen(head->data);
		head->PROTO_NUM=PutEvent(conn,event);
       		head->O_NODE=ntohl(LocalAddr(conn->Socket,NULL));
       	 	ret=SendPack(conn,head);
	}

        if(result) json_object_put(result);
        else json_object_put(err_json);
        json_object_put(json);

	return ret;
}
//ж��ģ��,svc��

extern int dmmgr_app(GDA *gp,char *model_name,JSON_OBJECT err_json);

int dmmgr(T_Connect *conn,T_NetHead *head)
{
T_SRV_Var *srvp=(T_SRV_Var *)conn->Var;
GDA *gp=(GDA *)srvp->var;
JSON_OBJECT err_json;
char msg[2048];
int ret,event=head->PROTO_NUM;

	err_json=json_object_new_array();
	conn->status=(head->ERRNO2==PACK_STATUS)?1:0;
	if(!head->PKG_LEN) {
		sprintf(msg,"data is empty!");
                json_object_array_add(err_json,jerr(100,msg));
		head->ERRNO1=LENGERR;
		head->ERRNO2=conn->status>0?PACK_NOANSER:-1;
                return_error(conn,head,json_object_to_json_string(err_json));
                conn->status=0;
                ShowLog(1,"%s:%s",__FUNCTION__,json_object_to_json_string(err_json));
                json_object_put(err_json);
		return 0;
	}

	ret=dmmgr_app(gp,head->data,err_json); //����Ӧ��ģ��

	if(head->ERRNO2 != PACK_NOANSER) {
	    if(ret)
		sprintf(msg,"model %s unloaded errno=%d,%s",head->data,errno,strerror(errno));
	    else
		sprintf(msg,"model %s is unloaded",head->data);
	    ShowLog(2,"%s:%s",__FUNCTION__,msg);
            json_object_array_add(err_json,jerr(ret,msg));
	
	    head->data=(char *)json_object_to_json_string(err_json);
	    head->ERRNO1=0;
	    head->ERRNO2=0;
	    head->PKG_LEN=strlen(head->data);
	    head->PROTO_NUM=PutEvent(conn,event);
            head->O_NODE=ntohl(LocalAddr(conn->Socket,NULL));
            ret=SendPack(conn,head);
	}
        json_object_put(err_json);

	return 0;
}