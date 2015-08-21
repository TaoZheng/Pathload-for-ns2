//#define MYDEBUG 1
#define PACKET_LEN 	25  // in bytes
#define TIME_INTERVAL 	.000100 // in usec
#define STREAM_LEN 	100 // packets
#define SENDER		1
#define RECEIVER	2
#define MAIN_T		1
#define SENDER_T		2

// pathload constants
#define MAX_INIT_RATE 		100
#define TR_MAX 			294
#define TR_MIN			1.5

#define RCV_TIMEOUT 		80000
#define NOTREND 		1
#define INCREASING		2
#define GREY			3

#define ACCEPTABLE_LOSS_RATE  	3
#define HIGH_LOSS_RATE 		15
#define RCVR_TIME_STAMP_ERROR   15 
#define MIN_STREAM_SZ 		5
#define GREY_BW_RESOLUTION	1
#define PCT_THRESHOLD		.55
#define PDT_THRESHOLD		.4
#define AGGREGATE_THRESHOLD	.7

// fleet parameter
#define NUM_STREAM		12
#define STREAM_LEN		100 /* # of packets */
#define MAX_STREAM_LEN		500 /* # of packets */
#define MIN_STREAM_LEN		40  /* # of packets */
#define FLEET_LEN 		MAX_STREAM_LEN*NUM_STREAM 
#define MAX_PACK_SZ 		147200  /* bytes */
#define MIN_PACK_SZ 		192    /* bytes */
// min time interval can be reduced further.
// do we need this now ?
#define MIN_TIME_INTERVAL	100    /* microsecond */

//#include <iostream.h> //被我注释掉了，貌似ns-2.27中不支持C＋＋标准STL
#include <math.h>
#include "ns-process.h"
#include "app.h"
#include "webcache/tcpapp.h"

//ostream &ostream_ = cerr  ; //此句无用
class TcpAgent;
class Pathload;

class PathloadTimer : public TimerHandler {
 public:
         PathloadTimer(Pathload* t) : TimerHandler(), t_(t) {}
         void expire(Event*);
 protected:
          Pathload* t_;
};


class StreamTimer : public TimerHandler {
 public:
         StreamTimer(Pathload* t) : TimerHandler(), t_(t) {}
         void expire(Event*);
 protected:
          Pathload* t_;
};


class Pathload : public Application {
public:
	Pathload();
	~Pathload();

	virtual void recv(int nbytes);
	virtual void send(int nbytes, AppData *data);

	void connect(Pathload *dst) { dst_ = dst; }

	virtual void process_data(int size, AppData* data);
	virtual AppData* get_data(int&, AppData*) {
		// Not supported
		abort();
		return NULL;
	}

	// Do nothing when a connection is terminated
	virtual void resume();

	void process_tcp_data(int size , AppData* data );
	void timeout(int tno);


	// pathload function
	int calc_param() ;
	void adjust_offset_to_zero(int owd[], int last_pkt_id);
	int rate_adjustment(int flag) ;
    int process_fleet( int owd[]   ) ;
    void analyze_trend(int median_owd[],int median_owd_len,float pdt_trend[],float pct_trend[],int *pct_result_cnt,int *pdt_result_cnt) ;
    int partition_stream(int pkt_cnt,int median_owd[],int owd_for_median_calc[] ) ;
    int get_stream(int low,int high,int owd_for_median_calc[] ) ;
    void order_int(int unord_arr[], int ord_arr[],int start, int num_elems);
    float  pairwise_diff_test(int array[] ,int start , int end) ;
    float pairwise_comparision_test(int array[] ,int start , int end) ;
    int aggregate_trend_result( float pct_trend[] , int pct_result_cnt, float pdt_trend[], int pdt_result_cnt) ;
    void radj_greymin() ;
    void radj_greymax() ;
    void radj_notrend()  ;
    void radj_increasing()  ;
    int converged() ;
    double grey_bw_resolution()  ;

//protected:
	virtual int command(int argc, const char*const* argv);
	CBuf* rcvr_retrieve_data() { return cbuf_.detach(); }
	void send_packet() ;

	// We don't have start/stop
	void start() ;  
	void stop();

    float pct_thresh , pdt_thresh , aggre_thresh ;
    int flag ; // set when actual rate is not same as requested rate
	
    FILE *trend_fp ; 
    int stream_finished ;
    int exp_stream_id ;

	Agent *tcpagent_ ;
	Agent *udpagent_ ;
	int type_ ;
	int running_ ;
	StreamTimer streamtimer_;
	PathloadTimer pathloadtimer_;

	Pathload *dst_;
	CBufList cbuf_;
	CBuf *curdata_;
	int curbytes_;
	int pkt_count;

	// fleet parameter
	long pkt_sz ;
	int time_interval ;
	int transmission_rate ;
    int num_stream ;
    int stream_len ; 


    int prev_trend  ; 
    int trend ;
	long fleet_id ;
	long stream_id ;
	long pkt_id ;
	double sndr_t , rcvr_t ;
	int owd[FLEET_LEN] ;
	int exp_flag  ;
	int grey_flag  ;
	double tr ;
	double tr_max ;
	double tr_min ;
	double grey_max  , grey_min  ;
	int num_stream_rcvd  ;
	int sock_tcp, sock_udp;
	long exp_fleet_id  ;
	double bw_resol;
	long min_time_interval, snd_latency, rcv_latency ;
	int toggle ;


	int totalBytes;
	
};

