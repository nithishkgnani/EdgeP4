/*************************************************************
Get and send position of Haptic Device tip.
Receive and apply forces.
Uses HDAPI only.
*************************************************************/

// OpenHaptic libraries
#include <HD/hd.h>
#include <HDU/hduVector.h>
#include <HDU/hduError.h>

/* Define keyboard hit library */
#include "conio.h"

#define Enable_Haptics 1 // 1 - for enable, 0 - for disable
int logging = 1;
#define HapticDeviceLogFilePath "/home/zenlab/Documents/Haptic_Testbed_Joy-master/TestbedPrograms/Operator/HapticDevice_GeomagicTouch/HapticGT_udp/Log_001_test.csv"
#define DataToLog "ID, PosX, PosY, PosZ, GripperClose, ButtonState2, Received_ID, PosRx, PosRy, PosRz\n"

/* Include the header file for Socket Communication */
#include "Communication.h"

/* Communication and data parameters */
#define PORTSEND 38005
#define PORTRECV 38005
#define SendtoUDPSocket_IP "10.0.0.2"/*"10.114.64.248"*/  
#define ReceiveUDPSocket_IP "10.0.0.1"/*"10.114.64.70"*/ 
int setup_sending_socket;
int setup_receive_socket;
/* Data structures to send and receive */
inline int16_t swap_int16(int16_t val){
    return (val<<8)|((val>>8)&(0xFF));
}
typedef struct payloadrecv_t
{
    int16_t IDr ;
    int16_t RobotID ;
    int16_t RoboPos[3];
    int16_t RoboQuats[4] ;
    int16_t ButtonState[2] ;
    int16_t FSRs[15] ;
    int16_t key1 ;
    int16_t key2 ;
    uint64_t ingress_global_timestamp ;
    uint64_t current_global_timestamp ;
    int16_t packet_length ;
    int16_t ingress_port ;
    int16_t egress_port ;
    int16_t dummy;
} payload_recv __attribute__((scalar_storage_order("big-endian")));

typedef struct payloadsend_t
{ // this structure is 4-byte aligned
    uint16_t IDs ;
    uint16_t RobotID ;
    int16_t GTPosition[3] ;
    int16_t RoboQuats[4] ;
    int16_t ButtonState[2] ;
    int16_t FSRs[15] ;
    int16_t key1 ;
    int16_t key2 ;
    uint64_t ingress_global_timestamp ;
    uint64_t current_global_timestamp ;
    int16_t packet_length ;
    int16_t ingress_port ;
    int16_t egress_port ;
    int16_t dummy;
} payload_send __attribute__((scalar_storage_order("big-endian")));

static payload_send payload_send_msg;
char bufferrecv[MAXLINE];

char buffersend[sizeof(payload_send_msg)];
int receivedbytesize = 0;
float x_send = 0, y_send = 0, z_send = 0;
/* Data structures to send and receive */

/* Define clock timestamp */
int64_t Timer_Start = 0,
        Timer_Loop = 0,
        Time_to_receive = 0;

void mainLoop();

/* Holds data retrieved from HDAPI. */
typedef struct
{
    HDboolean m_buttonState_1;          /* Has the device button been pressed. */
    HDboolean m_buttonState_2;          /* Has the extra device button been pressed. */
    hduVector3Dd m_devicePosition;      /* Current device coordinates in mm. */
    hduVector3Dd m_devicePosition_prev; /* Previous device coordinates in mm. */
    hduVector3Dd m_force;               /* Force. */
    HDErrorInfo m_error;
} DeviceData;

int16_t ReceivedData[3] = {0, 0, 0};
static int16_t roboQuats[4] = {0, 0, 0, 0};
static uint16_t fsrData[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Structure to hold Haptic Device data */
static DeviceData gServoDeviceData;

/* Callback handle for Force */
static HDdouble gMaxStiffness = 1.0;
static HDdouble maxForce = 1.0;
static HDdouble maxContForce = 1.0;
static HDdouble forceMag = 0;
HDSchedulerHandle gCallbackHandle = 0;

/******************************************************************************
  Main scheduler callback for getting position, velocity, joint angles,
  gimbal angles button state and rendering the received force.
  So, BeginFrame() and EndFrame() here.
 *****************************************************************************/
HDCallbackCode HDCALLBACK FForceCallback(void *pUserData)
{

    int nButtons = 0;
    HDErrorInfo error;

    hduVector3Dd force = {0, 0, 0};
    hduVector3Dd penetrationDistance;
    /** HAPTIC FRAME **/
    hdBeginFrame(hdGetCurrentDevice());

    /* Retrieve the current button(s). */
    hdGetIntegerv(HD_CURRENT_BUTTONS, &nButtons);
    /* In order to get the specific button 1 state, we use a bitmask to
       test for the HD_DEVICE_BUTTON_1 bit. and similarly for button 2 */
    gServoDeviceData.m_buttonState_1 = (nButtons & HD_DEVICE_BUTTON_1) ? HD_TRUE : HD_FALSE;
    gServoDeviceData.m_buttonState_2 = (nButtons & HD_DEVICE_BUTTON_2) ? HD_TRUE : HD_FALSE;

    /* Get the current location of the device (HD_GET_CURRENT_POSITION)
    We declare a vector of three doubles since hdGetDoublev returns
    the information in a vector of size 3. */
    hdGetDoublev(HD_CURRENT_POSITION, gServoDeviceData.m_devicePosition);
    /* Similarly get other Haptic Device data */
    hdGetDoublev(HD_LAST_POSITION, gServoDeviceData.m_devicePosition_prev);

    hdSetDoublev(HD_CURRENT_FORCE, force); // Send default {0,0,0} force

    if (Enable_Haptics)
    {
        /* Compute spring force as F = k * (anchor - pos), which will attract
           the device position towards the anchor position */

        hduVector3Dd forceDirection = {0, 0, 0};
        HDdouble penetrationDistanceX = gServoDeviceData.m_devicePosition[0];
        HDdouble penetrationDistanceY = gServoDeviceData.m_devicePosition[1] + 65.5107;
        HDdouble penetrationDistanceZ = gServoDeviceData.m_devicePosition[2] + 88.1142;
        hduVecSet(penetrationDistance, penetrationDistanceX, penetrationDistanceY, penetrationDistanceZ);

        HDdouble BoundXYZ[6] = {105, -105, 250, -24, 125, -250};

        if (penetrationDistanceX > BoundXYZ[0])
        {
            // forceDirection[0] = -1;
            forceDirection[0] = (BoundXYZ[0] - penetrationDistanceX) * 0.1;
        }
        else if (penetrationDistanceX < BoundXYZ[1])
        {
            // forceDirection[0] = -1;
            forceDirection[0] = (penetrationDistanceX - BoundXYZ[1]) * 0.1;
        }
        if (penetrationDistanceY > BoundXYZ[2])
        {
            // forceDirection[1] = -1;
            forceDirection[1] = (BoundXYZ[2] - penetrationDistanceY) * 0.001;
        }
        else if (penetrationDistanceY < BoundXYZ[3])
        {
            // forceDirection[1] = -1;
            forceDirection[1] = (penetrationDistanceY - BoundXYZ[3]) * 0.1; // higher force due to floor touch
        }
        if (penetrationDistanceZ > BoundXYZ[4])
        {
            // forceDirection[2] = -1;
            forceDirection[2] = (BoundXYZ[4] - penetrationDistanceZ) * 0.1;
        }
        else if (penetrationDistanceZ < BoundXYZ[5])
        {
            // forceDirection[2] = -1;
            forceDirection[2] = (penetrationDistanceZ - BoundXYZ[5]) * 0.001;
        }
        // printf("\n[DistanceX, Y, Z]: [%5.0f, %5.0f, %5.0f], [%5.0f, %5.0f, %5.0f]\n",
        //  penetrationDistanceX, penetrationDistanceY, penetrationDistanceZ,
        //  forceDirection[0], forceDirection[1], forceDirection[2]);

        hduVecScaleNonUniformInPlace(forceDirection, penetrationDistance);
        // hduVecScaleInPlace(forceDirection, penetrationDistance);
        // hduVecScaleInPlace(forceDirection, gMaxStiffness/2);
        //  printf("[DistanceX, Y, Z]: [%5.0f, %5.0f, %5.0f], [%5.0f, %5.0f, %5.0f] scaled\n",
        //   penetrationDistanceX, penetrationDistanceY, penetrationDistanceZ,
        //   forceDirection[0], forceDirection[1], forceDirection[2]);

        forceMag = hduVecMagnitude(forceDirection);

        hdSetDoublev(HD_CURRENT_FORCE, forceDirection); // Actual 

        /* Copy the position into our device_data tructure.
                And set the force values */
    }

    hdEndFrame(hdGetCurrentDevice());

    /* Check if an error occurred while attempting to render the force */
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        if (hduIsForceError(&error))
        {
            hdDisable(HD_FORCE_OUTPUT);
        }
        else if (hduIsSchedulerError(&error))
        {
            return HD_CALLBACK_DONE;
        }
    }

    return HD_CALLBACK_CONTINUE;
}

/*******************************************************************************
 Checks the state of the gimbal button and gets the position of the device.
*******************************************************************************/
HDCallbackCode HDCALLBACK copyDeviceDataCallback(void *pUserData)
{
    DeviceData *pDeviceData = (DeviceData *)pUserData;

    memcpy(pDeviceData, &gServoDeviceData, sizeof(DeviceData));

    return HD_CALLBACK_DONE;
}

/******************************************************************************
 Main loop. But runs only once.
Communication Parameter Setup
******************************************************************************/
void mainLoop()
{
    /* Communication setup */
    DeviceData currentData;

    setup_sending_socket = 0, setup_receive_socket = 0;
    setup_sending_socket = Get_UDP_SendSocket();
    FillDestinationPortInfo(SendtoUDPSocket_IP, PORTSEND); // uint16_t(PORTSEND);
    // setup_receive_socket = Get_UDP_RecvSocket(ReceiveUDPSocket_IP, PORTRECV);
    setup_receive_socket=setup_sending_socket;

    HDdouble stiffness = gMaxStiffness;
    int keypress;

    printf("Press Q to quit.\n\n");

    printf("Current Spring stiffness: %.3f N/mm\n", stiffness);

    /* Create file and open it */
    FILE *HapticDeviceDataLogFilePointer;
    if (fopen(HapticDeviceLogFilePath, "w+") != NULL)
    {
        logging = 1;
        HapticDeviceDataLogFilePointer = fopen(HapticDeviceLogFilePath, "w+");
        fprintf(HapticDeviceDataLogFilePointer, DataToLog);
    }
    else
    {
        printf("Can't find file");
    }

    //    float previousPos = 0; // To store previous y-axis position
    //    float diff = 0; // To store position difference in y-axis
    while (HD_TRUE)
    {

        /* Start loop timer */
        Timer_Start = s_clock();
        while (s_clock() - Timer_Start < 1000000) // wait 1 second (1000000 microseconds)
        {
        }
        // printf("Start Time: %ld us\n", Timer_Start);

        /* Perform a synchronous call to copy the most current device state.
           This synchronous scheduler call ensures that the device state
           is obtained in a thread-safe manner. */
        hdScheduleSynchronous(copyDeviceDataCallback, &currentData, HD_MIN_SCHEDULER_PRIORITY);

        // Static initializes variable to 0(given ID value) ONCE at the start of the loop
        static uint16_t sID = 0, rID = 0, Received_ID = 0, robotID = 0;
        /* Check for keyboard input. */
        if (_kbhit()) // To quit the program, press "Q"
        {
            keypress = getch();
            keypress = toupper(keypress);

            if (keypress == 'Q')
            {
                break;
            }
        }

        /* Send block */
        payload_send_msg.IDs = sID;
        payload_send_msg.RobotID = rID;
        x_send = currentData.m_devicePosition[0] * 10;
        y_send = (currentData.m_devicePosition[1] + 65.5107) * 10;
        z_send = (currentData.m_devicePosition[2] + 88.1142) * 10;
        payload_send_msg.GTPosition[0] = __builtin_bswap16((u_int16_t)x_send);
        payload_send_msg.GTPosition[1] = __builtin_bswap16((u_int16_t)y_send);
        payload_send_msg.GTPosition[2] = __builtin_bswap16((u_int16_t)z_send);
        payload_send_msg.ButtonState[0] = __builtin_bswap16(currentData.m_buttonState_1);
        payload_send_msg.ButtonState[1] = __builtin_bswap16(currentData.m_buttonState_2);

        memcpy(buffersend, &payload_send_msg, sizeof(payload_send));
        payload_send *psend = (payload_send *)buffersend;
        int sentbytes = send_payload(psend, sizeof(payload_send));
        sID += 1;
        rID = 328;

        /* Receive Block */
        if ((receivedbytesize = recv_payload(bufferrecv)) != 0)
        // 0 - Continue if no data received,
        // Any other integer - Continue only after receiving
        {
            bufferrecv[receivedbytesize] = '\0';
            payload_recv *precv = (payload_recv *)bufferrecv;
            Time_to_receive = s_clock() - Timer_Start;
            Received_ID = precv->IDr;
            robotID = precv->RobotID;
            ReceivedData[0] = ((int16_t)precv->RoboPos[0]);
            ReceivedData[1] = ((int16_t)precv->RoboPos[1]);
            ReceivedData[2] = ((int16_t)precv->RoboPos[2]);
            roboQuats[0] = __builtin_bswap16((int16_t)precv->RoboQuats[0]);
            roboQuats[1] = __builtin_bswap16((int16_t)precv->RoboQuats[1]);
            roboQuats[2] = __builtin_bswap16((int16_t)precv->RoboQuats[2]);
            roboQuats[3] = __builtin_bswap16((int16_t)precv->RoboQuats[3]);
            fsrData[0] = __builtin_bswap16((int16_t)precv->FSRs[0]);
            fsrData[1] = __builtin_bswap16((int16_t)precv->FSRs[1]);
            fsrData[2] = __builtin_bswap16((int16_t)precv->FSRs[2]);
            fsrData[3] = __builtin_bswap16((int16_t)precv->FSRs[3]);
            fsrData[4] = __builtin_bswap16((int16_t)precv->FSRs[4]);
            fsrData[5] = __builtin_bswap16((int16_t)precv->FSRs[5]);
            fsrData[6] = __builtin_bswap16((int16_t)precv->FSRs[6]);
            fsrData[7] = __builtin_bswap16((int16_t)precv->FSRs[7]);
            fsrData[8] = __builtin_bswap16((int16_t)precv->FSRs[8]);
            fsrData[9] = __builtin_bswap16((int16_t)precv->FSRs[9]);
            fsrData[10] = __builtin_bswap16((int16_t)precv->FSRs[10]);
            fsrData[11] = __builtin_bswap16((int16_t)precv->FSRs[11]);
            fsrData[12] = __builtin_bswap16((int16_t)precv->FSRs[12]);
            fsrData[13] = __builtin_bswap16((int16_t)precv->FSRs[13]);
            fsrData[14] = __builtin_bswap16((int16_t)precv->FSRs[14]);
            gServoDeviceData.m_force[0] = (int16_t)precv->RoboPos[0]; // has to be force, but temporarily changed
            gServoDeviceData.m_force[0] = (int16_t)precv->RoboPos[1];
            gServoDeviceData.m_force[0] = (int16_t)precv->RoboPos[2];

            memset(bufferrecv, '\0', MAXLINE);
        }

        /* Check if the main scheduler callback has exited. */
        if (!hdWaitForCompletion(gCallbackHandle, HD_WAIT_CHECK_STATUS))
        {
            fprintf(stderr, "\nThe main scheduler callback has exited\n");
            fprintf(stderr, "\nPress any key to quit.\n");
            getch();
            return;
        }

        /* Write data to file */
        Timer_Loop = s_clock() - Timer_Start;
        // if (logging == 1)
        // {
        //     fprintf(HapticDeviceDataLogFilePointer,
        //             "%d, %d, %f, %f, %f, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
        //             psend->IDs, psend->RobotID,
        //             payload_send_msg.GTPosition[0], payload_send_msg.GTPosition[1], payload_send_msg.GTPosition[2],
        //             payload_send_msg.ButtonState[0], payload_send_msg.ButtonState[1], payload_send_msg.RoboQuats[0],
        //             payload_send_msg.RoboQuats[1], payload_send_msg.RoboQuats[2], payload_send_msg.RoboQuats[3],
        //             payload_send_msg.FSRs[15], payload_send_msg.FSRs[15], payload_send_msg.FSRs[15], payload_send_msg.FSRs[15],
        //             payload_send_msg.FSRs[15], payload_send_msg.FSRs[15], payload_send_msg.FSRs[15], payload_send_msg.FSRs[15],
        //             payload_send_msg.FSRs[15], payload_send_msg.FSRs[15], payload_send_msg.FSRs[15], payload_send_msg.FSRs[15],
        //             payload_send_msg.FSRs[15], payload_send_msg.FSRs[15], payload_send_msg.FSRs[15]);
        //             // Received_ID, ReceivedData[0], ReceivedData[1], ReceivedData[2]);
        // }

        if (sID % 1 == 0)
        {
            // printf("\n[Start Time, Time_to_receive, Timer_Loop]: [%ld, %5.0ld, %5.0ld] us\n",
            //     Timer_Start, Time_to_receive, Timer_Loop);
            printf("\nGTPos: [%d %d %d] - sID:%d - RobotID:%d\n",
                   swap_int16(psend->GTPosition[0]), swap_int16(psend->GTPosition[1]), swap_int16(psend->GTPosition[2]),
                   psend->IDs, psend->RobotID);
            printf("ButtonState: [%d] [%d] \n",
                   __builtin_bswap16(psend->ButtonState[0]), __builtin_bswap16(psend->ButtonState[1]));
            printf("Quat: %d %d %d %d\nFSR : %d %d %d\nFSR : %d %d %d\nFSR : %d %d %d\nFSR : %d %d %d\nFSR : %d %d %d\n",
                   psend->RoboQuats[0], psend->RoboQuats[1], psend->RoboQuats[2], psend->RoboQuats[3],
                   psend->FSRs[0], psend->FSRs[1], psend->FSRs[2],
                   psend->FSRs[3], psend->FSRs[4], psend->FSRs[5],
                   psend->FSRs[6], psend->FSRs[7], psend->FSRs[8],
                   psend->FSRs[9], psend->FSRs[10], psend->FSRs[11],
                   psend->FSRs[12], psend->FSRs[13], psend->FSRs[14]);
            
            printf("Received_GTPos: %d %d %d - RecvID:%d - RobotID:%d [Received Bytes: %d]\n", 
            swap_int16(ReceivedData[0]), swap_int16(ReceivedData[1]), swap_int16(ReceivedData[2]), Received_ID, robotID, receivedbytesize);
            printf("Quat: %d %d %d %d\nFSR : %d %d %d\nFSR : %d %d %d\nFSR : %d %d %d\nFSR : %d %d %d\nFSR : %d %d %d\n",
            roboQuats[0], roboQuats[1], roboQuats[2], roboQuats[3], fsrData[0], fsrData[1], fsrData[2], fsrData[3], fsrData[4],
            fsrData[5], fsrData[6], fsrData[7], fsrData[8], fsrData[9], fsrData[10], fsrData[11], fsrData[12], fsrData[13], 
            fsrData[14]);
            // printf("RecvID:%d [Received Bytes: %d]\n", Received_ID, receivedbytesize);
        }
        // s_sleep(1000);
    }

    /* Close the file */
    // if (logging == 1)
    // {
    //     fclose(HapticDeviceDataLogFilePointer);
    // }
    /* Terminate the network socket connections */
    printf("Connection terminate\n");
    closeSocket(setup_receive_socket);

    return;
}

/******************************************************************************
 Main function.
******************************************************************************/
int main(int argc, char *argv[])
{
    HDErrorInfo error;
    HHD hHD = hdInitDevice(HD_DEFAULT_DEVICE);
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "Failed to initialize haptic device");
        fprintf(stderr, "\nPress any key to quit.\n");
        getch();
        return -1;
    }

    printf("Demo for Force-Feedback of Tactile Testbed.\n");

    /* Schedule the haptic callback function for continuously monitoring the
       button state and rendering the anchored spring force. */
    gCallbackHandle = hdScheduleAsynchronous(
        FForceCallback, 0, HD_MAX_SCHEDULER_PRIORITY);

    hdEnable(HD_FORCE_OUTPUT);

    HDdouble tyy, tyu;
    /* Query the max closed loop control stiffness that the device
       can handle. Using a value beyond this limit will likely cause the
       device to buzz. */
    hdGetDoublev(HD_NOMINAL_MAX_STIFFNESS, &gMaxStiffness);
    hdGetDoublev(HD_NOMINAL_MAX_FORCE, &maxForce);
    hdGetDoublev(HD_NOMINAL_MAX_CONTINUOUS_FORCE, &maxContForce);
    printf("Max Stiffness: -> %lf \n", gMaxStiffness); // 0.5
    printf("Max Force: -> %lf \n", maxForce);          // 3.3
    printf("Max Cont Force: -> %lf \n", maxContForce); // 0.88
    /* Start the haptic rendering loop. */
    hdStartScheduler();
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "Failed to start scheduler");
        fprintf(stderr, "\nPress any key to quit.\n");
        getch();
        return -1;
    }

    /* Start the main application loop. */
    mainLoop();

    /* Cleanup by stopping the haptics loop, unscheduling the asynchronous
       callback, disabling the device. */
    hdStopScheduler();
    hdUnschedule(gCallbackHandle);
    hdDisableDevice(hHD);

    return 0;
}
