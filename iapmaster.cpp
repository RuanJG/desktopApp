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
        if( len != 3 )
            break;
        mAckPkg.type = data[1];
        mAckPkg.value = data[2];
        mAckPkg.valid = true;
        break;
    }

    default:
        break;

    }
}



void IapMaster::iapTick(int intervalMs)
{
    if( mStep == STEP_IDEL )
        return;

    mTimeOutMs += intervalMs;

    if( mStep == STEP_START ){
        if( mAckPkg.valid ){
            if( mAckPkg.type == PACKGET_ACK_OK && mAckPkg.value == PACKGET_START_ID ){
                //this is a ack for start packget
                setStep( STEP_DATA );
                iapEvent(EVENT_TYPE_STARTED, "");
                mAckPkg.valid = false;
                mTimeOutMs = 0;
                //return;
            }
        }
        if( mTimeOutMs >= 500 ){
            //make sure 500 ms is enought time for ack comeback
            sendStartPackget();
            //iapEvent(EVENT_TYPE_STATUS, "."  );
            mTimeOutMs = 0;
        }
    }

    if( mStep == STEP_DATA ){
        if( mCurrentSize == 0 ){
            size_t len  = 100;
            sendDataPackget( mFirmwareBuffer+mCurrentSize , len );
            mCurrentSize += len;
        }
        if( mAckPkg.valid ){
            if( mAckPkg.type == PACKGET_ACK_OK && mAckPkg.value == PACKGET_DATA_ID ){
                //this is a ack for data packget
                size_t len  = 100;
                len =  len <= (mFirmwareSize - mCurrentSize) ? len:(mFirmwareSize - mCurrentSize);
                if( len > 0){
                    sendDataPackget( mFirmwareBuffer+mCurrentSize , len );
                    mCurrentSize += len;

                    std::stringstream  sss;
                    sss << "send " << mCurrentSize << "/" << mFirmwareSize ;
                    iapEvent(EVENT_TYPE_STATUS, sss.str()  );
                }else{
                    //finish
                    setStep( STEP_JUMP );
                    sendJumpPackget();
                }

                mTimeOutMs = 0;
                //return;
            }else{

                iapStop();

                string res;
                if( mAckPkg.type == PACKGET_ACK_FALSE ){
                    switch(mAckPkg.value){
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
                    defalut:{
                        stringstream ss;
                        ss << "unknow ack=" << mAckPkg.type << "," <<mAckPkg.value ;
                        res = ss.str();
                        break;
                    }
                    }

                    iapEvent(EVENT_TYPE_FALSE, res);

                }else if ( mAckPkg.type == PACKGET_ACK_OK ){
                    //res = "Unknow Ack receive";
                    //maybe is the start packget ack
                    stringstream ss;
                    ss << "unknow ack=" << mAckPkg.type << "," <<mAckPkg.value ;
                    res = ss.str();
                    iapEvent(EVENT_TYPE_STATUS, res);
                }

            }
        }
        mAckPkg.valid =false;
        if( mTimeOutMs >= 3000 ){
            //3s timeout , lost connect
            iapStop();
            iapEvent(EVENT_TYPE_FALSE, "Timeout");
        }
    }

    if( mStep == STEP_JUMP ){
        if( mAckPkg.valid ){
            if( mAckPkg.type == PACKGET_ACK_OK && mAckPkg.value == PACKGET_END_ID ){
                //this is a ack for start packget
                setStep( STEP_IDEL );
                iapEvent(EVENT_TYPE_FINISH, "");
                mTimeOutMs = 0;
                mAckPkg.valid = false;
                //return;
            }
        }
#if 0
        if( mTimeOutMs >= 200 ){
            sendJumpPackget();
            mTimeOutMs = 0;
        }
#else
        if( mTimeOutMs >= 1000 ){
            //1s timeout , lost connect
            iapStop();
            iapEvent(EVENT_TYPE_FALSE, "Timeout");
            mTimeOutMs = 0;
        }
#endif
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

void IapMaster::sendDataPackget(unsigned char *data, int len)
{
    Chunk chunk;
    chunk.append(PACKGET_DATA_ID);
    chunk.append( ((++mCurrentDataSeq)%PACKGET_MAX_DATA_SEQ) );
    chunk.append(data,len);

    sendPackget( (unsigned char *) chunk.getData(), chunk.getSize());
}


void IapMaster::sendJumpPackget()
{
    unsigned char buffer[2];
    buffer[0] = PACKGET_END_ID;
    buffer[1] = PACKGET_END_JUMP;
    sendPackget(buffer,2);
}
