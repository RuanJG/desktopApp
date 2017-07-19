#include "iapmaster.h"
#include <sstream>

//id
#define PACKGET_START_ID  1 //[id start]data{[0]}
#define PACKGET_ACK_ID  2 //[id ack]data{[ACK_OK/ACK_FALSE/ACK_RESTART][error code/packget id]}
#define PACKGET_DATA_ID 3 //[id data] data{[seq][]...[]}
#define PACKGET_END_ID 4 //[id end] data{[stop/jump]}


//data
#define PACKGET_ACK_NONE  0
#define PACKGET_ACK_OK 1
#define PACKGET_ACK_FALSE 2
#define PACKGET_ACK_RESTART 3

#define PACKGET_END_JUMP 1
#define PACKGET_END_STOP 2

#define PACKGET_MAX_DATA_SEQ 200  // new_seq = (last_seq+1)% PACKGET_MAX_DATA_SEQ, seq init 0 , first seq should =1

// ack error code
#define PACKGET_ACK_FALSE_PROGRAM_ERROR  21// the send data process stop
#define PACKGET_ACK_FALSE_ERASE_ERROR 22 // the send data process stop
#define PACKGET_ACK_FALSE_SEQ_FALSE  23 // the send data process will restart
// if has ack false , the flash process shuld be restart

using namespace std;


IapMaster::IapMaster():
    mFirmwarePath(),
    mFirmwareSize(0),
    mCurrentSize(0),
    mFirmwareBuffer(NULL),
    mStep(STEP_IDEL),
    mDecoder(),
    mEncoder(),
    mCurrentDataSeq(0),
    mTimeOutMs(0),
    mMutex()
{

}


IapMaster::~IapMaster()
{
    iapStop();
}


bool IapMaster::isIapStarted()
{
    return ( mStep != STEP_IDEL );
}

bool IapMaster::iapStart(string filepath)
{
    if( filepath.empty() ){
        iapEvent(EVENT_TYPE_FALSE, "iap FilePath is NULL");
        return false;
    }
    mFirmwarePath = filepath;

    ifstream ifs;
    ifs.open(mFirmwarePath,ios::in|ios::binary);
    if( !ifs.is_open()){
        iapEvent(EVENT_TYPE_FALSE, "iap FilePath can not open");
        return false;
    }

    ifs.seekg(0, ios::end);
    mFirmwareSize = ifs.tellg();
    ifs.seekg(0);
    if( mFirmwareSize <= 0){
        ifs.close();
        iapEvent(EVENT_TYPE_FALSE, "iap File is 0 byte");
        return false;
    }

    if(mFirmwareBuffer)
        delete[] mFirmwareBuffer;
    mFirmwareBuffer = new unsigned char[mFirmwareSize];

    ifs.read( (char*)mFirmwareBuffer, mFirmwareSize);
    size_t count = ifs.gcount();
    if( mFirmwareSize != count ){
         stringstream ss;
         ss<< "read iap File " << count << "bytes, total "<< mFirmwareSize <<" bytes";
         iapEvent(EVENT_TYPE_FALSE, ss.str());

         ifs.close();
         delete[] mFirmwareBuffer;
         mFirmwareBuffer = NULL;
         mFirmwareSize = 0;
         return false;
    }

    ifs.close();
    iapReset();

    stringstream ss;
    ss<< "read iap File " << mFirmwareSize <<" bytes";
    iapEvent(EVENT_TYPE_STATUS, ss.str());

    //iap start packget
    sendStartPackget();
    iapEvent(EVENT_TYPE_STATUS, "iap start..."  );
    mTimeOutMs = 200;

    return true;
}




void IapMaster::iapReset()
{
    mCurrentSize = 0;
    mCurrentDataSeq = 0;
    mStep = STEP_START;
    mAckPkg.valid = false;
}


void IapMaster::iapStop()
{
    if( NULL != mFirmwareBuffer )
        delete[] mFirmwareBuffer;
    mFirmwareBuffer = NULL;
    setStep(STEP_IDEL);
}

void IapMaster::setStep(int step)
{
    mMutex.lock();
    mStep = step;
    mMutex.unlock();
}

float IapMaster::getIapStatus()
{
    return ((float)mCurrentSize/(float)mFirmwareSize);
}


void IapMaster::iapParse (unsigned char *data, size_t len)
{
    std::list<Chunk> packgetList;

    mDecoder.decode( data, len, packgetList);

    std::list<Chunk>::iterator iter;
    for (iter = packgetList.begin(); iter != packgetList.end(); ++iter) {
        const unsigned char *p = iter->getData();
        int cnt = iter->getSize();
        handle_device_message( p,cnt );
    }
}



void IapMaster::handle_device_message ( const unsigned char *data, int len)
{
    switch(data[0]){

    case PACKGET_ACK_ID:{
        // ack from devices
        if( len != 3 )
            break;
        mAckPkg.type = data[1];
        mAckPkg.value = data[2];
        iapServer(mAckPkg);
        break;
    }

    default:
        break;

    }
}


void IapMaster::iapServer( IapAckPackget &ack )
{
    if( ack.type == PACKGET_ACK_FALSE ){
        string res;

        switch(ack.value){
        case PACKGET_ACK_FALSE_ERASE_ERROR:{
            res = "Erase false";
            break;
        }
        case PACKGET_ACK_FALSE_PROGRAM_ERROR:{
            res = "program false";
            break;
        }
        case PACKGET_ACK_FALSE_SEQ_FALSE:{
            res = "seq false";
            break;
        }
        default:{
            stringstream ss;
            ss << "iap : unknow false ack=" << ack.type << "," <<ack.value ;
            res = ss.str();
            break;
        }
        }

        iapEvent(EVENT_TYPE_FALSE, res);
        iapStop();
        return;
    }


    if( ack.type == PACKGET_ACK_RESTART ){
        iapReset();
        sendStartPackget();
        iapEvent(EVENT_TYPE_STATUS, "iap restart..."  );
        mTimeOutMs = 200;
        return;
    }

    if( ack.type == PACKGET_ACK_OK ){

        switch (mStep) {
        case STEP_START:{
            if( PACKGET_START_ID == ack.value ){
                mCurrentSize = 0 ;
                mCurrentDataSeq = 0;
                sendDataPackget();
                mStep = STEP_DATA;
                mTimeOutMs = 1000;
                iapEvent(EVENT_TYPE_STARTED, "");
            }else{
                stringstream ss;
                ss << "iap : STEP_START unknow ack" ;
                iapEvent(EVENT_TYPE_STATUS, ss.str()  );
            }
            break;
        }
        case STEP_DATA:{
            if( PACKGET_DATA_ID == ack.value ){
                if( mCurrentSize == mFirmwareSize ){
                    //to jump step
                    sendJumpPackget();
                    mStep = STEP_JUMP;
                    mTimeOutMs = 1000;
                }else{
                    //send next data
                    sendDataPackget();
                    mTimeOutMs = 1000;
                    std::stringstream  sss;
                    sss << "send " << mCurrentSize << "/" << mFirmwareSize ;
                    iapEvent(EVENT_TYPE_STATUS, sss.str()  );
                }
            }else{
                stringstream ss;
                ss << "iap : STEP_DATA unknow ack" ;
                iapEvent(EVENT_TYPE_STATUS, ss.str()  );
            }
            break;
        }
        case STEP_JUMP:{
            if( PACKGET_END_ID == ack.value ){
                setStep( STEP_IDEL );
                iapEvent(EVENT_TYPE_FINISH, "");
                iapStop();
            }else{
                stringstream ss;
                ss << "iap : STEP_JUMP unknow ack" ;
                iapEvent(EVENT_TYPE_STATUS, ss.str()  );
            }
            break;
        }

        default:{
            stringstream ss;
            ss << "iap : error unknow STEP_JUMP " ;
            iapEvent(EVENT_TYPE_FALSE, ss.str()  );
            iapStop();
            break;
        }
        }

        return;
    }
}


void IapMaster::iapTick(int intervalMs)
{
    if( ! isIapStarted() )
        return;

    mTimeOutMs -= intervalMs;

    if( mTimeOutMs <= 0 ){
        if( mStep == STEP_START){
            sendStartPackget();
            mTimeOutMs = 200;
        }else{
            iapStop();
            iapEvent(EVENT_TYPE_FALSE, "Timeout");
        }
    }
}




void IapMaster::sendPackget( unsigned char *data, int len)
{
    Chunk pChunk;
    if ( mEncoder.encode( data, len ,pChunk) ){
        iapSendBytes( (unsigned char *)pChunk.getData(),pChunk.getSize());
    }
}

void IapMaster::sendStartPackget()
{
    unsigned char buffer[2];
    buffer[0] = PACKGET_START_ID;
    buffer[1] = 0;
    sendPackget(buffer,2);
}

void IapMaster::sendDataPackget()
{
    size_t len  = 100;
    len =  len <= (mFirmwareSize - mCurrentSize) ? len:(mFirmwareSize - mCurrentSize);
    if( len <= 0 )
        return;

    Chunk chunk;
    chunk.append(PACKGET_DATA_ID);
    chunk.append( ((++mCurrentDataSeq)%PACKGET_MAX_DATA_SEQ) );
    chunk.append(mFirmwareBuffer+mCurrentSize , len);
    mCurrentSize += len;

    sendPackget( (unsigned char *) chunk.getData(), chunk.getSize());
}


void IapMaster::sendJumpPackget()
{
    unsigned char buffer[2];
    buffer[0] = PACKGET_END_ID;
    buffer[1] = PACKGET_END_JUMP;
    sendPackget(buffer,2);
}
