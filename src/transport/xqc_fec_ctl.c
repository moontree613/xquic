
#include <xquic/xquic_typedef.h>
#include <xquic/xquic.h>
#include "src/transport/xqc_multipath.h"
#include "src/transport/xqc_utils.h"
#include "src/transport/xqc_conn.h"
#include <math.h>

void
xqc_fec_update_fec_ratio(xqc_path_ctx_t *path){
    /*更新fec添加比例*/
    size_t lost_and_repair_cnt = 0;
    //size_t lost_cnt = 0;
    size_t lost_cnt = 0;
    size_t repair_cnt = 0;
    size_t tmp = 0;
    for(size_t i=0;i<FEC_OB_WND_SIZE;i++){
        /*if(path->fec_ob_wnd[i] == XQC_FEC_PKT_STATUS_REPAIR || path->fec_ob_wnd[i] == XQC_FEC_PKT_STATUS_LOST){
            lost_and_repair_cnt++;
        }*/
        if(path->fec_ob_wnd[i] == XQC_FEC_PKT_STATUS_LOST){
            lost_cnt++;
        }
        if(path->fec_ob_wnd[i] == XQC_FEC_PKT_STATUS_REPAIR){
            repair_cnt++;
        }
    }//检查fec ob window中丢包、repaired的情况
    if ((lost_cnt + repair_cnt)==0){
        //此时不丢包
        //path->FEC_N = 0;
        /*todo：关闭fec*/
    }
    else{
        tmp = FEC_OB_WND_SIZE/(lost_cnt + repair_cnt);
        printf("lost_cnt = %lu\n",lost_cnt);
        printf("repair_cnt = %lu\n",repair_cnt);
        printf("path->FEC_N = %lu\n",FEC_OB_WND_SIZE/(lost_cnt + repair_cnt));
        path->FEC_N = tmp;//repair_cnt + lost_cnt, C语言中/默认下取整，会以更保守的方式保护数据,FEC_OB_WND_SIZE/(lost_and_repair_cnt)
    }
    return;
};
void
xqc_fec_ob_wnd_update_on_packet_acked(xqc_connection_t *conn, xqc_packet_out_t *acked_packet){
    xqc_path_ctx_t *path = xqc_conn_find_path_by_path_id(conn, acked_packet->po_path_id);
    if (path == NULL) {
        xqc_log(conn->log, XQC_LOG_WARN, "|can't find path by id|%ui|", acked_packet->po_path_id);
        return;
    }
    if(acked_packet->po_pkt.pkt_num > path->fec_ob_wnd_upper_bound){
        /*更新upper~pkt_num直接全部包的状态*/
        for(size_t i=path->fec_ob_wnd_upper_bound + 1 ;i<acked_packet->po_pkt.pkt_num;i++){//需要+1，从上个upperbound的下一个包开始更新标记，否则覆盖上个最大ack的包状态
            path->fec_ob_wnd[i % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_UNACKED;
        };
        path->fec_ob_wnd[acked_packet->po_pkt.pkt_num % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_ACKED;
        //更新upperbound
        path->fec_ob_wnd_upper_bound = acked_packet->po_pkt.pkt_num;

    }
    else if(acked_packet->po_pkt.pkt_num == path->fec_ob_wnd_upper_bound){
        /*更新pkt_num包的状态*/
        path->fec_ob_wnd[acked_packet->po_pkt.pkt_num % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_ACKED;
    }
    else{//小于的情况
        if(acked_packet->po_pkt.pkt_num < path->fec_ob_wnd_upper_bound - FEC_OB_WND_SIZE){//小于观察窗口下界，不做处理
            return;
        }
        path->fec_ob_wnd[acked_packet->po_pkt.pkt_num % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_ACKED;
    }
    /*更新观察窗后要更新比例*/
    xqc_fec_update_fec_ratio(path);
    return;
};

void
xqc_fec_ob_wnd_update_on_packet_lost(xqc_connection_t *conn, xqc_packet_out_t *lost_packet){//xqc_fec_ob_wnd_update_on_packet_lost埋在两个地方：超时重传检测 和 冗余重传检测
    xqc_path_ctx_t *path = xqc_conn_find_path_by_path_id(conn, lost_packet->po_path_id);
    if (path == NULL) {
        xqc_log(conn->log, XQC_LOG_WARN, "|can't find path by id|%ui|", lost_packet->po_path_id);
        return;
    }
    if(lost_packet->po_pkt.pkt_num > path->fec_ob_wnd_upper_bound){//最新的一个包丢失
        /*更新upper~pkt_num直接全部包的状态*/
        for(size_t i=path->fec_ob_wnd_upper_bound + 1 ;i<lost_packet->po_pkt.pkt_num;i++){
            path->fec_ob_wnd[i % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_UNACKED;
        };
        path->fec_ob_wnd[lost_packet->po_pkt.pkt_num % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_LOST;
        //更新upperbound
        path->fec_ob_wnd_upper_bound = lost_packet->po_pkt.pkt_num;

    }
    else if(lost_packet->po_pkt.pkt_num == path->fec_ob_wnd_upper_bound){
        /*更新pkt_num包的状态*/
        path->fec_ob_wnd[lost_packet->po_pkt.pkt_num % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_LOST;
    }
    else{//小于的情况
        if(lost_packet->po_pkt.pkt_num < path->fec_ob_wnd_upper_bound - FEC_OB_WND_SIZE){//小于观察窗口下界，不做处理
            return;
        }
        path->fec_ob_wnd[lost_packet->po_pkt.pkt_num % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_LOST;
    }
    /*更新观察窗后要更新比例*/
    printf("update fec ratio due to packet_lost\n");
    xqc_fec_update_fec_ratio(path);
    return;
};

void
xqc_fec_ob_wnd_update_on_packet_repaired(xqc_connection_t *conn, uint64_t repair_pkt_num, uint64_t path_id){
    xqc_path_ctx_t *path = xqc_conn_find_path_by_path_id(conn, path_id);
    if (path == NULL) {
        xqc_log(conn->log, XQC_LOG_WARN, "|can't find path by id|%ui|", path_id);
        return;
    }
    if(repair_pkt_num > path->fec_ob_wnd_upper_bound){//最新的一个包丢失
        /*更新upper~pkt_num直接全部包的状态*/
        for(size_t i=path->fec_ob_wnd_upper_bound + 1 ;i<repair_pkt_num;i++){
            path->fec_ob_wnd[i % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_UNACKED;
        };
        path->fec_ob_wnd[repair_pkt_num % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_REPAIR;
        //更新upperbound
        path->fec_ob_wnd_upper_bound = repair_pkt_num;

    }
    else if(repair_pkt_num == path->fec_ob_wnd_upper_bound){
        /*更新pkt_num包的状态*/
        path->fec_ob_wnd[repair_pkt_num % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_REPAIR;
    }
    else{//小于的情况
        if(repair_pkt_num < path->fec_ob_wnd_upper_bound - FEC_OB_WND_SIZE){//小于观察窗口下界，不做处理
            return;
        }
        path->fec_ob_wnd[repair_pkt_num % FEC_OB_WND_SIZE] = XQC_FEC_PKT_STATUS_REPAIR;
    }
    /*更新观察窗后要更新比例*/
    printf("update fec ratio due to packet_repaired\n");
    xqc_fec_update_fec_ratio(path);
    return;
};
