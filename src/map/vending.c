#include <stdio.h>
#include <string.h>

#include "clif.h"
#include "itemdb.h"
#include "map.h"
#include "vending.h"
#include "pc.h"
#include "skill.h"
#include "battle.h"
#include "nullpo.h"
#include "chat.h"
#include "npc.h"
#include "trade.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

/*==========================================
 * 露店閉鎖
 *------------------------------------------
*/
void vending_closevending(struct map_session_data *sd)
{

	nullpo_retv(sd);

	sd->vender_id=0;
	clif_closevendingboard(&sd->bl,0);
}

/*==========================================
 * 露店アイテムリスト要求
 *------------------------------------------
 */
void vending_vendinglistreq(struct map_session_data *sd,int id)
{
	struct map_session_data *vsd;

	nullpo_retv(sd);

	if( (vsd=map_id2sd(id)) == NULL )
		return;
	if(vsd->vender_id == 0 || vsd->deal_mode != 0)
		return;
	if(sd->vender_id != 0 || sd->deal_mode != 0)
		return;
	clif_vendinglist(sd,id,vsd->vending);
}

/*==========================================
 * 露店アイテム購入
 *------------------------------------------
 */
void vending_purchasereq(struct map_session_data *sd,int len,int id,unsigned char *p)
{
	int i,j,new=0,blank,vend_list[12];
	short amount,index;
	struct map_session_data *vsd=map_id2sd(id);
	double w,z;

	nullpo_retv(sd);

	blank=pc_inventoryblank(sd);

	if(vsd==NULL || vsd->vender_id==0)
		return;

	for(i=0,w=z=0;8+4*i<len;i++){
		amount=*(short*)(p+4*i);
		index=*(short*)(p+2+4*i)-2;
/*
		if(amount < 0) return;	//add
		for(j=0;j<vsd->vend_num;j++)
			if(0<vsd->vending[j].amount && amount<=vsd->vending[j].amount && vsd->vending[j].index==index)
				break;
*/
//ADD_start
		for(j=0;j < vsd->vend_num;j++) {
			if(0 < vsd->vending[j].amount && vsd->vending[j].index==index) {
				if(amount > vsd->vending[j].amount || amount <= 0) {
					clif_buyvending(sd,index,vsd->vending[j].amount,4);
					return;
				}
				if(amount <= vsd->vending[j].amount) break;
			}
		}
//ADD_end
		if(j==vsd->vend_num)
			return;	// 売り切れ
		vend_list[i]=j;
		z+=(double)vsd->vending[j].value*amount;
		if(z > (double)sd->status.zeny){
			clif_buyvending(sd,index,amount,1);
			return;	// zeny不足
		}
		w+=(double)itemdb_weight(vsd->status.cart[index].nameid)*amount;
		if(w+(double)sd->weight > (double)sd->max_weight){
			clif_buyvending(sd,index,amount,2);
			return;	// 重量超過
		}
		switch(pc_checkadditem(sd,vsd->status.cart[index].nameid,amount)){
		case ADDITEM_EXIST:
			break;
		case ADDITEM_NEW:
			new++;
			if(new > blank)
				return;	// 種類数超過
			break;
		case ADDITEM_OVERAMOUNT:
			return;	// アイテム数超過
		}
	}
	if(vsd==NULL || vsd->vender_id==0)
		return;
	pc_payzeny(sd,(int)z);
	pc_getzeny(vsd,(int)z);
	for(i=0;8+4*i<len;i++){
		amount=*(short*)(p+4*i);
		index=*(short*)(p+2+4*i)-2;
		if(amount < 0 || amount > vsd->status.cart[index].amount) break;	//add
		pc_additem(sd,&vsd->status.cart[index],amount);
		vsd->vending[vend_list[i]].amount-=amount;
		pc_cart_delitem(vsd,index,amount,0);
		clif_vendingreport(vsd,index,amount);
		if(battle_config.buyer_name){
			char temp[256];
			sprintf(temp,"購入者は %s さんです",sd->status.name);
			clif_disp_onlyself(vsd,temp,strlen(temp));
		}
	}
}

/*==========================================
 * 露店開設
 *------------------------------------------
 */
void vending_openvending(struct map_session_data *sd,int len,char *message,int flag,unsigned char *p)
{
	int i;

	nullpo_retv(sd);

	if(sd->deal_mode != 0) return;
	if(sd->npc_id)
		npc_event_dequeue(sd);
	if(sd->trade_partner)
		trade_tradecancel(sd);
	if(sd->chatID)
		chat_leavechat(sd,0);

	if(flag){
		for(i=0;85+8*i<len;i++){
			sd->vending[i].index=*(short*)(p+8*i)-2;
			sd->vending[i].amount=*(short*)(p+2+8*i);
			sd->vending[i].value=*(int*)(p+4+8*i);
			if(sd->vending[i].value>battle_config.vending_max_value)sd->vending[i].value=battle_config.vending_max_value;
			// カート内のアイテム数と販売するアイテム数に相違があったら中止
			// ついでに−値の値段や−値の個数チェックもいれてます
			if(sd->vending[i].value<0 || sd->vending[i].amount<0 || pc_cartitem_amount(sd,sd->vending[i].index,sd->vending[i].amount)<0){
				clif_skill_fail(sd,MC_VENDING,0,0);
				return;
			}
		}
		sd->vender_id=sd->bl.id;
		sd->vend_num=i;
		strncpy(sd->message,message,80);
		if(clif_openvending(sd,sd->vender_id,sd->vending) > 0)
			clif_showvendingboard(&sd->bl,message,0);
		else
			sd->vender_id=0;
	}
}

