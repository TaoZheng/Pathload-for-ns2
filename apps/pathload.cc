/*
 Author : Manish Jain
*/
#include "agent.h"
#include "app.h"
#include "pathload.h"

Pathload::Pathload() : running_(0), streamtimer_(this), pathloadtimer_(this)
{
    flag = 0 ;
    type_ = 0 ;
    curdata_ = NULL ; 
    curbytes_ = 0 ;
    pkt_count = 0;
    fleet_id = 0 ;
    tr_max = 0 ;
    tr_min = 0;
    grey_min = 0 ;
    grey_max = 0 ;
    exp_flag = 1;
    prev_trend = 0 ;
    trend = 0 ;
    num_stream = 0; 
    stream_len = 0 ;
    stream_finished = 0 ;
    totalBytes = 0;
    bind("transmission_rate", &transmission_rate);
    bind("bw_resol", &bw_resol);
    trend_fp = fopen("trend.log" , "w") ;

    for ( int i = 0 ; i < num_stream*stream_len ; i++ )
           owd[i] = 0 ;
    bind("type_", &type_);
    bind("num_stream",&num_stream); 
    bind("stream_len",&stream_len) ;
}

// ADU for plain Pathload, which is by default a string of otcl script
// XXX Local to this file
class PathloadString : public AppData {
private:
    int size_;
    char* str_; 
public:
    PathloadString() : AppData(TCPAPP_STRING), size_(0), str_(NULL) {}
    PathloadString(PathloadString& d) : AppData(d) {
        size_ = d.size_;
        if (size_ > 0) {
            str_ = new char[size_];
            strcpy(str_, d.str_);
        } else
            str_ = NULL;
    }
    virtual ~PathloadString() { 
        if (str_ != NULL) 
            delete []str_; 
    }

    char* str() { return str_; }
    virtual int size() const { return AppData::size() + size_; }

    // Insert string-contents into the ADU
    void set_string(const char* s) {
        if ((s == NULL) || (*s == 0)) 
            str_ = NULL, size_ = 0;
        else {
            size_ = strlen(s) + 1;
            str_ = new char[size_];
            assert(str_ != NULL);
            strcpy(str_, s);
        }
    }
    virtual AppData* copy() {
        return new PathloadString(*this);
    }
};

// Pathload
static class PathloadClass : public TclClass {
 public:
         PathloadClass() : TclClass("Application/Pathload") {}
         TclObject* create(int, const char*const*) {
                return (new Pathload);
         }
} class_app_pathload;


Pathload::~Pathload()
{
    // XXX Before we quit, let our agent know what we no longer exist
    // so that it won't give us a call later...
    agent_->attachApp(NULL);
}

// Send bytes over TCP channel
void Pathload::send(int nbytes, AppData *cbk)
{

//totalBytes += nbytes;

    CBuf *p = new CBuf(cbk, nbytes);
    cbuf_.insert(p);
    // tcpapp used the following to send data,
    // send in turn makes a call to sendmsg of
    // agent_ object.
    // well, in our case agent_object represnts
    // the udp object and not the tcp object.
    // so we will make a explicit call to
    // tcpagent_->sendmsg(int)
    tcpagent_->sendmsg(nbytes);
}


// recv bytes over TCP channel.
void Pathload::recv(int size)
{
totalBytes += size;
    // If it's the start of a new transmission, grab info from dest, 
    // and execute callback
    if (curdata_ == 0)
        curdata_ = dst_->rcvr_retrieve_data();

    if (curdata_ == 0) {
        //ostream_ << " no data " << endl ; 
        printf(" no data \n");
        return;
    }
    curbytes_ += size;

    if (curbytes_ == curdata_->bytes()) 
    {
        // We've got exactly the data we want
        // If we've received all data, execute the callback
        process_tcp_data(curdata_->size(), curdata_->data());
        // Then cleanup this data transmission
        delete curdata_;
        curdata_ = NULL;
        curbytes_ = 0;
    }
    else if (curbytes_ > curdata_->bytes()) 
    {
        // We've got more than we expected. Must contain other data.
        // Continue process callbacks until the unfinished callback
        while (curbytes_ >= curdata_->bytes()) 
        {
            process_tcp_data(curdata_->size(), curdata_->data());
            curbytes_ -= curdata_->bytes();
#ifdef MYDEBUG
            fprintf(stderr, 
                "[%g] %s gets data size %d(left %d)\n", 
                Scheduler::instance().clock(), 
                name(),
                curdata_->bytes(), curbytes_);
#endif
            delete curdata_;
            curdata_ = dst_->rcvr_retrieve_data();
            if (curdata_ != 0)
                continue;
            if ((curdata_ == 0) && (curbytes_ > 0)) 
            {
                fprintf(stderr, "[%g] %s gets extra data!\n",
                    Scheduler::instance().clock(), name_);
                Tcl::instance().eval("[Simulator instance] flush-trace");
                abort();
            } 
            else
                break;
        }
    }
}

// process recv'd TCP bytes.
void Pathload::process_tcp_data(int size , AppData* data )
{

    char buff[50] ;
    char *p ;

    strcpy(buff,(char*)((PacketData *)data)->data()); 
  if ( type_ == SENDER )
  {
    #ifdef MYDEBUG
    ostream_ <<"sender : receiving tcp data : " << buff << endl ; 
    #endif
    if ( strncmp(buff, "NEX",3) == 0 )
    {
        stream_id++ ;
        //ostream_ << "about to send stream = " << stream_id << endl ;
        if ( stream_id < num_stream )
        {
            double t = time_interval/1000000. ; // periodic interval
            streamtimer_.resched(t) ;
        }
    }
    else if ( strncmp(buff,"PKT",3) == 0 )
    {
        p = &buff[3] ; 
        sscanf(p, "%ld",&pkt_sz); //%d  ---->     %ld
#ifdef MYDEBUG
        ostream_ << "sender : pkt length = " << pkt_sz << endl ;
#endif
    }
    else if ( strncmp(buff,"TIN",3) == 0 )
    {
        p = &buff[3] ; 
        sscanf(p, "%d",&time_interval );
#ifdef MYDEBUG
        ostream_ << "sender : time interval = " << time_interval << endl ;
#endif

        // now that we have fleet param, 
        // send a new fleet.
        ++fleet_id ;
        stream_id = 0 ;
        pkt_id = 0 ;
        double t = time_interval/1000000. ; // periodic interval
        streamtimer_.sched(t) ;
    }
  }
  else if ( type_ == RECEIVER )
  {
      p = &buff[3] ; 
      int ltmp ; 
      sscanf(p, "%d",&ltmp );
      if ( ltmp == exp_stream_id )
      {
            stream_finished = 1 ;
            //send a tcp message to sender requesting
            // next stream
            stream_finished = 0 ;
            exp_stream_id++ ;
            PacketData *pkt_data = new PacketData(PACKET_LEN) ;
            strcpy((char *)pkt_data->data(),"NEX");
            send(PACKET_LEN,pkt_data);
            if ( ltmp == num_stream-1)
            {
                // received complete fleet
                // analyze trend in fleet 
                //adjust_offset_to_zero(owd, lpacket_id );  

                FILE *fp = fopen("one_way_delay.log" , "a") ;
                fprintf(fp , "For fleet id =  \n");
                for ( int li = 0 ; li < num_stream*stream_len ; li++ )
                    fprintf(fp , "%2d %d\n",li ,owd[li]);
                fprintf(fp , "\n\n");
                fclose(fp); 
                trend = process_fleet(owd);
                if (trend != GREY)
                {
                   if (exp_flag == 1 && prev_trend != 0 && prev_trend != trend)
                   {
                        exp_flag = 0;
                   }

                    prev_trend = trend;
                }
                if (rate_adjustment(trend) == -1)
                {   
                    PacketData *pkt_data = new PacketData(PACKET_LEN) ;
                    strcpy((char *)pkt_data->data(),"TER");
                    send(PACKET_LEN,pkt_data);
                    printf("Available bandwidth range[Rmin-Rmax] : %.2f - %.2f (Mbps)\n", tr_min, tr_max);
 //                   printf("Relative variation : %4.3g \n", (tr_max-tr_min)*2./(tr_max+tr_min));
                    fclose(trend_fp ) ;
                    Tcl::instance().evalf("%s appdone", this->name());
                    // we are done here. but calling exit is 
                    // not right.
               }
               else
                pathloadtimer_.resched(0);

            }
      }
  }
}

void Pathload::resume()
{
    // Do nothing
}

int Pathload::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();

    if (strcmp(argv[1], "connect") == 0) {
        dst_ = (Pathload *)TclObject::lookup(argv[2]);
        if (dst_ == NULL) {
            tcl.resultf("%s: connected to null object.", name_);
            return (TCL_ERROR);
        }
        dst_->connect(this);
        return (TCL_OK);
    } 
    else if (strcmp(argv[1], "send") == 0) {
        /*
         * <app> send <size> <tcl_script>
         */
        int size = atoi(argv[2]);
        if (argc == 3)
            send(size, NULL);
        else {
            PathloadString *tmp = new PathloadString();
            tmp->set_string(argv[3]);
            send(size, tmp);
        }
        return (TCL_OK);
    } 
    else if (strcmp(argv[1], "dst") == 0) {
        tcl.resultf("%s", dst_->name());
        return TCL_OK;
    }
    else if (strcmp(argv[1], "attach-udp-agent") == 0) 
    {
      #ifdef MYDEBUG
        ostream_ << "DEBUG command attach-udp-agent" << endl ;
      #endif

      agent_ = (Agent*) TclObject::lookup(argv[2]);
      if (agent_ == 0)
      {
        tcl.resultf("no such agent %s", argv[2]);
        return(TCL_ERROR);
      }
      udpagent_ = agent_ ;
      udpagent_->attachApp(this);
      return(TCL_OK);
    }
    else if (strcmp(argv[1], "attach-tcp-agent") == 0) 
    {
      #ifdef MYDEBUG
        ostream_ << "DEBUG command attach-tcp-agent" << endl ;
      #endif

      tcpagent_ = (Agent*) TclObject::lookup(argv[2]);
      if (tcpagent_ == 0) 
      {
        tcl.resultf("no such agent %s", argv[2]);
        return(TCL_ERROR);
      }
      tcpagent_->attachApp(this);
      return(TCL_OK);
    }
    return Application::command(argc, argv);
}


// added my code
void Pathload::start()
{
    if ( type_ == SENDER )
        //ostream_ << "starting sender " << endl ;
        printf("starting sender \n");
    else
    {
        //ostream_ << "starting receiver " << endl ;
        //ostream_ << "bw resol = " << bw_resol << endl ;
        printf("starting receiver \n");
        printf("bw resol =  %f\n", bw_resol);

        tr = transmission_rate/1000000. ;
    }
    if ( num_stream == 0 )
        num_stream = NUM_STREAM ;
    if ( stream_len == 0 )
        stream_len = STREAM_LEN ;

    printf("from pathload : num_stream = %d stream_len = %d \n", num_stream, stream_len ) ;
    running_ = 1;
    double t = 0 ;
    pathloadtimer_.sched(t);
}

void Pathload::stop()
{
        running_ = 0;
}


void Pathload::timeout(int tno)
{
   int packet_len ;
   char buff[25] ;

   if ( tno == MAIN_T )
   {

    if ( type_ == SENDER && running_ )
    {
        #ifdef MYDEBUG
        ostream_ << "sender : inside timeout - should we be in here ?" << endl ;
        #endif 
    }
    else if (type_ == RECEIVER && running_) 
    {
        
        #ifdef MYDEBUG
        ostream_ << "receiver : sending tcp data" << endl ;
        #endif 

        // sending tcp data
        // pakcet len (L)
        for (int i = 0 ; i < num_stream*stream_len ; i++ )
               owd[i] = 0 ;
        transmission_rate = (int)rint(tr * 1000000) ; 
        calc_param() ;
        PacketData *pkt_data = new PacketData(PACKET_LEN) ;
        strcpy((char *)pkt_data->data(),"PKT");
        sprintf(buff, "%ld",pkt_sz);
        strcat((char *)pkt_data->data(),buff) ; 
        send(PACKET_LEN,pkt_data);

        // time interval (T)
        pkt_data = new PacketData(PACKET_LEN) ;
        bzero(buff,25); 
        sprintf(buff, "%d", time_interval) ;//%ld  ---->     %d
        strcpy ((char *)pkt_data->data() , "TIN") ;
        strcat ((char *)pkt_data->data() , buff ) ;
        send(PACKET_LEN,pkt_data);

        exp_stream_id = 0 ;
        
        #ifdef MYDEBUG 
        ostream_ << "receiver :  sent "<< PACKET_LEN << " bytes at "
           << Scheduler::instance().clock() << endl ;
        #endif 
    }
   }
    else if ( tno == SENDER_T )
    {
        // start sending stream ( at periodic intervals )
        send_packet() ;
        pkt_id++ ;
        if ( pkt_id < (stream_id+1)*stream_len )
        {
          double t = time_interval/1000000. ; // periodic interval
          streamtimer_.resched(t) ;
        }
        else
        {
            //send stream finished STF 
            PacketData *pkt_data = new PacketData(PACKET_LEN) ;
            bzero(buff,25); 
            strcpy ((char *)pkt_data->data() , "STF") ;
            sprintf(buff, "%ld", stream_id) ;//%d  ---->     %ld
            strcat ((char *)pkt_data->data() , buff ) ;
            send(PACKET_LEN,pkt_data);
        }
    }
}

//send 1 packet over UDP
void Pathload::send_packet()
{

    char buff[1500] ;
    PacketData *pkt_data = new PacketData(pkt_sz) ;
    double s_t , s_t_f = 0 ; 
    bzero(buff,25); 
    memcpy(buff,&fleet_id , sizeof(long)) ;
    memcpy(buff+sizeof(long),&stream_id , sizeof(long)) ;
    memcpy(buff+2*sizeof(long),&pkt_id , sizeof(long)) ;

    if ( pkt_id == stream_id * stream_len )
    {
        s_t_f  = Scheduler::instance().clock() ;
        s_t = s_t_f ;
    }
    else 
        s_t =  Scheduler::instance().clock() - s_t_f  ;
    long s_t_i  = (long) rint( s_t * 1000000 ) ;
    memcpy(buff+3*sizeof(long), &s_t_i ,  sizeof(long));
    memcpy((char*)((PacketData *)pkt_data)->data(),buff,25) ;
    udpagent_->sendmsg(pkt_sz,pkt_data);

    #ifdef  MYDEBUG
        ostream_ << "sender : sending 1 udp data at "
                << Scheduler::instance().clock() << endl ;
    #endif
}


// recv data over UDP 
//udp agent checks if app is present and calls
//process_data method of app to "deliver" data
//to app.
//process_data is defined in class process 

void Pathload::process_data(int size , AppData* data )
{
    long int lfleet_id ;
    long int lstream_id ;
    long int lpacket_id ;
    char buff[25] ;
    long sndr_t_i ;

totalBytes += size;

    #ifdef MYDEBUG
    //ostream_ << "receiver : receiving udp data " << endl ;
    printf("receiver : receiving udp data \n");
    #endif
    if ( data == NULL )
        //cout << " data NULL " << endl ;
        printf(" data NULL ");
    else
    {
        bzero(buff,25) ;
        memcpy(buff, (char *)((PacketData *)data)->data(),25);
        memcpy(&lfleet_id , buff , sizeof(long)) ;
        memcpy(&lstream_id , buff+sizeof(long) , sizeof(long) ) ;
        memcpy(&lpacket_id , buff+2*sizeof(long) , sizeof(long) ) ;
        memcpy(&sndr_t_i, buff+3*sizeof(long),sizeof(long)) ;
        double rcv_t =  Scheduler::instance().clock() ;
        owd[lpacket_id] =  (int)rint(rcv_t*1000000 - sndr_t_i) ;
        if ( lpacket_id == (lstream_id+1)*stream_len - 1 || stream_finished )
        {
            //send a tcp message to sender requesting
            // next stream
            stream_finished = 0 ;
            exp_stream_id++ ;
            PacketData *pkt_data = new PacketData(PACKET_LEN) ;
            strcpy((char *)pkt_data->data(),"NEX");
            send(PACKET_LEN,pkt_data);
            if ( lstream_id == num_stream-1)
            {
                // received complete fleet
                // analyze trend in fleet 
                FILE *fp = fopen("one_way_delay.log" , "a") ;
                fprintf(fp , "For fleet id = %ld \n",lfleet_id); //%d  ---->     %ld
                for ( int li = 0 ; li < lpacket_id ; li++ )
                    fprintf(fp , "%2d %d\n",li ,owd[li]);
                fprintf(fp , "\n\n");
                fclose(fp); 
                trend = process_fleet(owd);
                if (trend != GREY)
                {
                   if (exp_flag == 1 && prev_trend != 0 && prev_trend != trend)
                   {
                        exp_flag = 0;
                   }

                    prev_trend = trend;
                }
                if (rate_adjustment(trend) == -1)
                {   
                    PacketData *pkt_data = new PacketData(PACKET_LEN) ;
                    strcpy((char *)pkt_data->data(),"TER");
                    send(PACKET_LEN,pkt_data);
                    printf("Available bandwidth range[Rmin-Rmax] : %.2f - %.2f (Mbps)\n", tr_min, tr_max);
//                    printf("Relative variation : %4.3g \n", (tr_max-tr_min)*2./(tr_max+tr_min));
                    fclose(trend_fp ) ;
                    Tcl::instance().evalf("%s appdone", this->name());
                    // we are done here. but calling exit is 
                    // not right.
               }
               else
                pathloadtimer_.resched(0);

            }
        }
    }
}


void PathloadTimer::expire(Event*)
{
        t_->timeout(MAIN_T);
}

void StreamTimer::expire(Event*)
{
        t_->timeout(SENDER_T);
}


/*
  calculates fleet param L,T .
  calc_param returns -1, if we have 
  reached to upper/lower limits of the 
  stream parameters like L,T,tr .
  0 otherwise.
*/
int Pathload:: calc_param()
{
    int mod_val ;

    time_interval = MIN_TIME_INTERVAL ;
    pkt_sz =(int) rint( transmission_rate * (time_interval / 8000000.)) ;
//printf("pkt_sz = %d\n", pkt_sz);
    if ( pkt_sz < MIN_PACK_SZ )
    {
        pkt_sz = MIN_PACK_SZ ;
        printf("setting pkt sz to minimum \n"); 
        time_interval = (long) rint(( pkt_sz ) * 8000000. / transmission_rate ) ;
    
        if ( pkt_sz >= MAX_PACK_SZ )
        {
            printf("Pkt Size limit reached. Cannot proceed any further. Terminating ...\n");
            return -1 ;
        }

    }
//    printf("    receiver : calc_param : T = %d us L = %ld byte R = %.2f mbps\n", time_interval,pkt_sz, tr) ;//%d  ---->     %ld
	printf("%f  %.2fM %fM\n", Scheduler::instance().clock(), tr, totalBytes/1000000.0) ;
    return 0 ;
}


/* Adjust offset to zero again  */
void Pathload::adjust_offset_to_zero(int owd[], int last_pkt_id)
{
    int owd_min = 0;
    int i ; 
    for (i=0; i<=last_pkt_id; i++) {
        if ( owd_min == 0 && owd[i] != 0 ) owd_min=owd[i];
        else if (owd_min != 0 && owd[i] != 0 && owd[i]<owd_min) owd_min=owd[i];
    }

    for (i=0; i<=last_pkt_id; i++) {
        if ( owd[i] != 0 )
            owd[i] -= owd_min;
    }
}



int Pathload::get_stream(int low,int high,int owd_for_median_calc[] )
{
    int j ,lpkt_cnt=0 ;
	for ( j = low ; j <= high ; j++ )
	{
		if ( owd[j]  )
		{
			owd_for_median_calc[lpkt_cnt++]  = owd[j];
		}
	} 
	return lpkt_cnt ;
}


int Pathload::partition_stream(int pkt_cnt,int median_owd[],int owd_for_median_calc[] )
{
	int j=0 , lmedian_owd_len=0 ;
	int lpkt_per_min=0;
	int lcount ;
	int ordered[MAX_STREAM_LEN];

	lpkt_per_min = (int)floor(sqrt((double)pkt_cnt));
	lcount = 0 ;
	for ( j = 0 ; j < pkt_cnt  ; j=j+lpkt_per_min )
	{
		if ( j+lpkt_per_min >= pkt_cnt )
		{
			lcount = pkt_cnt - j ;
		}
		else
		{
			lcount = lpkt_per_min;
		}
		order_int(owd_for_median_calc , ordered ,j,lcount ) ;
		if ( lcount % 2 == 0 )
		{
			median_owd[lmedian_owd_len]= (ordered[(int)(lcount*.5)-1]+ordered[(int)(lcount*0.5)] )/2 ;
		}
		else
		{
			median_owd[lmedian_owd_len] =  ordered[(int)(lcount*0.5)] ;
		}
		lmedian_owd_len++ ;

	}
	return lmedian_owd_len ;
}


void Pathload::analyze_trend(int median_owd[],int median_owd_len,float pdt_trend[],float pct_trend[],int *pct_result_cnt,int *pdt_result_cnt)
{
	float ret_val ;
	if ( (ret_val = pairwise_comparision_test(median_owd , 0 , median_owd_len )) != -1)
	{
		pct_trend[*pct_result_cnt] = ret_val ;
        
		fprintf(trend_fp , "pct trend : stream=%d  metric=%f\n", *pct_result_cnt ,pct_trend[*pct_result_cnt]);
		(*pct_result_cnt)++ ;
	}

	if ( (ret_val = pairwise_diff_test(median_owd , 0 , median_owd_len )) != 2 )
	{
		pdt_trend[*pdt_result_cnt] = ret_val ;
	    fprintf(trend_fp ,  "pdt trend : stream=%d  metric=%f\n", *pdt_result_cnt,pdt_trend[*pdt_result_cnt] );
		(*pdt_result_cnt)++ ;
	}
}

/*
  looks for trend in a fleet.
  returns : trend in the fleet.
*/
#define TREND_ARRAY_LEN 50 
int Pathload::process_fleet( int owd[]   )
{
	int median_owd[FLEET_LEN] ;
	int owd_for_median_calc[MAX_STREAM_LEN] ;
	float  pdt_trend[TREND_ARRAY_LEN] ,pct_trend[TREND_ARRAY_LEN] ;
	int flag = 1;
	int i , j ;
	int median_owd_len  = 0 , median_split_len = 0  , split_idx = 0 ;
	int pkt_per_min ;
	int count , low = 0 , high = 0 ;
	int pkt_cnt = 0 ;
	int  median_split[MAX_STREAM_LEN];
	int start = 0 ;
	int pct_result_cnt=0, pdt_result_cnt=0 ;
	int trend ;

	/*
	 To decide the 'fleet trend', we compute the trend in each stream
	 and aggregate it.
	*/

    low = 0 ;
	for ( i=0 ; i< num_stream ; i++ )
	{
		high = (i+1)*stream_len-1;
		pkt_cnt = get_stream(low,high,owd_for_median_calc) ;
		if ( pkt_cnt > MIN_STREAM_LEN )
		{
			median_owd_len=partition_stream(pkt_cnt,median_owd,owd_for_median_calc );
            FILE *fp = fopen("filtered_owd.log" , "a") ;
            fprintf(fp, "stream = %d \n", i ) ;
            for ( int li = 0 ; li < median_owd_len ; li++ )
                fprintf(fp , "%2d %d\n",li, median_owd[li]);
            fclose(fp); 

			analyze_trend(median_owd,median_owd_len,pdt_trend,pct_trend,&pct_result_cnt,&pdt_result_cnt);
		}
		median_owd_len=0;
		low = stream_len * ( i+1) ;
	}

	trend = aggregate_trend_result(pct_trend,pct_result_cnt,pdt_trend,pdt_result_cnt);
	return trend ;
}

void Pathload::order_int(int unord_arr[], int ord_arr[],int start, int num_elems)
{
	int i,j,k;
	int temp;
	for (i=start,k=0;i<start+num_elems;i++,k++) ord_arr[k]=unord_arr[i];
	for (i=1;i<num_elems;i++) {
		for (j=i-1;j>=0;j--)
			if (ord_arr[j+1] < ord_arr[j]) {
				temp=ord_arr[j]; 
				ord_arr[j]=ord_arr[j+1]; 
				ord_arr[j+1]=temp;
			}
			else break;
	}
}

/*
	PCT test to detect increasing trend in stream
*/
float Pathload::pairwise_comparision_test (int array[] ,int start , int end)
{
	int improvement = 0 ,i ;
	int  total ;

    

	if ( ( end - start  ) >= MIN_STREAM_SZ )
	{
		for ( i = start ; i < end -1   ; i++ )
		{
			if ( array[i] < array[i+1] )
			{
				improvement += 1 ;
			}
		}
		total = ( end - start - 1) ;
        float ret_val = ((float)improvement)/total ;
		return ( ret_val ) ;
	}
	else
	{
		return -1 ;
	}
}

/*
	PDT test to detect increasing trend in stream
*/
float Pathload::pairwise_diff_test(int array[] ,int start , int end)
{
	float y = 0 , y_abs = 0 ;
	int i ;

	if ( ( end - start  ) >= MIN_STREAM_SZ )
	{
		for ( i = start+1 ; i < end    ; i++ )
		{
			y += array[i] - array[i-1] ;
			y_abs += fabsf((float)array[i] - (float)array[i-1]) ;
		}
        if ( y_abs == 0 )
               return -1 ;
        else
		    return y/y_abs ;
	}
	else
	{
		return 2. ;
	}
}

int Pathload::aggregate_trend_result( float pct_trend[] , int pct_result_cnt, float pdt_trend[], int pdt_result_cnt)
{
int total=0 ,i_cnt = 0, n_cnt = 0;
int dscrd=0;
int i=0;

/* mark each stream as increasing/decreasing based
   on pct and pdt metric. if the two metric show
   conflicting trend, discard the stream.
*/
//printf("    Trend per stream[%2d]:: ",pct_result_cnt); 
fprintf(trend_fp , "    Trend per stream[%2d]:: ",pct_result_cnt); 
for (i=0; i < pct_result_cnt;i++ )
{
		
	if ( pct_trend[i] == -1 || pdt_trend[i] == 2 )
	{
//		printf("d");
		fprintf(trend_fp,"d");
		dscrd++ ;
	}
	else if ( pct_trend[i] >= PCT_THRESHOLD && pdt_trend[i] >= PDT_THRESHOLD )
	{
//		printf("I");
		fprintf(trend_fp,"I");
		i_cnt++;
	}
	else if ( pct_trend[i] < PCT_THRESHOLD && pdt_trend[i] < PDT_THRESHOLD )
	{
//		printf("N");
		fprintf(trend_fp,"N");
		n_cnt++;
	}
	else if ( pct_trend[i] >= PCT_THRESHOLD && pdt_trend[i] <= PDT_THRESHOLD*1.1 && pdt_trend[i] >= PDT_THRESHOLD*.9 )
	{
//		printf("I");
		fprintf(trend_fp,"I");
		i_cnt++;
	}
	else if ( pdt_trend[i] >= PDT_THRESHOLD && pct_trend[i] <= PCT_THRESHOLD*1.1 && pct_trend[i] >= PCT_THRESHOLD*.9 )
	{
//		printf("I");
		fprintf(trend_fp,"I");
		i_cnt++;
	}
	else if ( pct_trend[i] < PCT_THRESHOLD && pdt_trend[i] <= PDT_THRESHOLD*1.1 && pdt_trend[i] >= PDT_THRESHOLD*.9 )
	{
//		printf("N");
		fprintf(trend_fp,"N");
		n_cnt++;
	}
	else if ( pdt_trend[i] < PDT_THRESHOLD && pct_trend[i] <= PCT_THRESHOLD*1.1 && pct_trend[i] >= PCT_THRESHOLD*.9 )
	{
//		printf("N");
		fprintf(trend_fp,"N");
		n_cnt++;
	}
	else
	{
//		printf("U");
		fprintf(trend_fp,"U");
	}
	total++ ;
}
//printf("\n"); 
fprintf(trend_fp,"\n");
total-=dscrd ;

if( (double)i_cnt/(total) >= AGGREGATE_THRESHOLD )
{
//	printf("    Aggregate trend     :: INCREASING\n");
	fprintf(trend_fp ,"    Aggregate trend     :: INCREASING\n");
	return INCREASING ;
}
else if( (double)n_cnt/(total) >= AGGREGATE_THRESHOLD )
{
//	printf("    Aggregate trend     :: NO TREND\n");
	fprintf(trend_fp,"    Aggregate trend     :: NO TREND\n");
	return NOTREND ;
}
else 
{
//	printf("    Aggregate trend     :: GREY\n");
	fprintf(trend_fp,"    Aggregate trend     :: GREY\n");
	return GREY ;
}
printf("\n") ;
fprintf(trend_fp,"\n") ;

}

/*
  return max allowed grey bw range.
  need something better here !!
*/
double Pathload::grey_bw_resolution() 
{
	if ( (tr_min + tr_max)/2. <= 20  )
	{
		return GREY_BW_RESOLUTION ;
	}
	else 
	{
		return 2*GREY_BW_RESOLUTION ;
	}
}

/*
	test if Rmax and Rmin range is smaller than
	user specified bw resolution
	or
	if Gmin and Gmax range is smaller than grey
	bw resolution.
*/
int Pathload::converged()
{

	if ( tr_max != 0 && tr_max != tr_min && ( tr_max - tr_min <= bw_resol || 
	( tr_max - grey_max <= grey_bw_resolution() && grey_min - tr_min <= grey_bw_resolution() ) ) )
	{
		if ( tr_max - tr_min <= bw_resol )
		{
			printf("%f %f - %f-------%fM\nExiting due to user specified resolution----\n", Scheduler::instance().clock(), tr_min, tr_max, totalBytes/1000000.0);
		}
		else
		{
			printf("%f %f - %f-------%fM\nExiting due to grey region----\n", Scheduler::instance().clock(), tr_min, tr_max, totalBytes/1000000.0);
		}		
		fflush(stderr) ;
		return 1 ;
	}
	else
	{
		return 0 ;
	}
}

/*
	Calculate next fleet rate
	when fleet showed INCREASING trend. 
*/
void Pathload::radj_increasing() 
{

      if (exp_flag)
      {
          if ( !grey_max  )
            tr = tr / 2. ;
          else if ( tr_max - grey_max >= grey_bw_resolution() )
                tr = ( tr_max + grey_max)/2. ;
          else //rmx-gmx is close enuf, lets try to estimate rmn-gmn
              tr = grey_min/2. ;
      }
      else
      {
        if ( grey_max != 0 && grey_max >= tr_min )
        {
            if ( tr_max - grey_max <= grey_bw_resolution() )
            {
                radj_notrend() ;
            }
            else 
            {
                tr = ( tr_max + grey_max)/2. ;
            }
        }
        else
        {
            tr =  ( tr_max + tr_min)/2. ;
        }
      }
   
}


/*
    Calculate next fleet rate
    when fleet showed NOTREND trend. 
*/
void Pathload::radj_notrend() 
{
    if ( flag)
    {
        flag = 0 ;
        if ( grey_min !=  0 )
            tr =  ( tr_min + grey_min ) / 2. ;
        else if ( grey_max != 0 )
            tr = ( tr_min + grey_max ) / 2. ;
        else
            tr = ( tr_max + tr_min ) / 2. ;
    }
    else 
    {

      if ( exp_flag )
      {
        tr =  2 * tr ;
      }
      else
      {
        if ( grey_min != 0 && grey_min <= tr_max )
        {
            if ( grey_min - tr_min <= grey_bw_resolution() )
            {
                radj_increasing() ;
            }
            else 
            {
                tr = ( tr_min + grey_min ) / 2. ;
            }
        }
        else
        {
            tr =  ( tr_max + tr_min)/2. ;
        }
      }
    }
}


/*
    Calculate next fleet rate
    when fleet showed GREY trend. 
*/
void Pathload::radj_greymax()
{

    if ( tr_max == 0 )
    {
        tr = tr  + .5 * tr ;
    }
    else if ( tr_max - grey_max <= grey_bw_resolution() )
    {
        radj_greymin() ;
    }
    else 
    {
        tr = ( tr_max + grey_max)/2. ;
    }
}

/*
    Calculate next fleet rate
    when fleet showed GREY trend. 
*/
void Pathload::radj_greymin()
{
    if ( grey_min - tr_min <= grey_bw_resolution() )
    {
        radj_greymax() ;
    }
    else 
    {
        tr = ( tr_min + grey_min ) / 2. ;
    }
}
/*
	dpending upon trend in fleet :-
	- update the state variables.
	- decide the next fleet rate
*/
int Pathload::rate_adjustment(int flag)
{

	if( flag == INCREASING )
	{
		if ( grey_max >= tr )
			grey_max = grey_min = 0 ;

		tr_max = tr ;
		printf("    Rmax                :: %.2fMbps\n",tr_max);

		if ( !converged() ) 
		{
			radj_increasing() ;
		}
		else
		{
			return -1; 
		}
	}
	else if ( flag == NOTREND )
	{
		if ( grey_min < tr )
			grey_min = 0 ;
		if ( grey_max < tr )
			grey_max = grey_min = 0 ;

		tr_min =  tr ;
		printf("    Rmin                :: %.2fMbps\n",tr_min);
		if ( !converged() )
		{
			radj_notrend() ;
		}
		else
		{
			return -1; 
		}
	}
	else if ( flag == GREY )
	{
		if ( grey_max == 0 && grey_min == 0 )
		{
			grey_max =  grey_min = tr ;
		}

		if (tr==grey_max || tr>grey_max )
		{
	 	  grey_max = tr ;
		  printf("    Gmax                :: %.2fMbps\n",grey_max);
		  if ( !converged() )
		  {
		  	radj_greymax() ;
		  }
		  else
		  {
		  	return -1 ;
		  }
		}
		else if ( tr < grey_min || grey_min == 0  ) 
		{
			grey_min = tr ;
			printf("    Gmin                :: %.2fMbps\n",grey_min);
			if ( !converged() )
			{
				radj_greymin() ;
			}
			else
			{
				return -1 ;
			}
		}

	}

	if ( tr > MAX_PACK_SZ*8./min_time_interval && toggle == 1 )
	{
		/* u have hit the max limit */
		tr_max = MAX_PACK_SZ*8./min_time_interval ;
		return -1 ;
	}
	else if ( tr > MAX_PACK_SZ*8./min_time_interval && toggle == 0 )
	{
		toggle = 1 ;
		tr = MAX_PACK_SZ*8./min_time_interval ;
	}
	transmission_rate = (long) rint(1000000 * tr) ;
	return 0 ;
}
