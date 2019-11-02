#define SERVO_0 13
#define SERVO_1 12
#define SERVO_2 14
#define SERVO_3 27
#define SERVO_4 26
#define SERVO_5 25
#define SERVO_TIME_BUFFER 15

Servo servo0;
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
Servo servo5;

int pwmServoPos = 0;
int pwmServoGoal = 0;
int servoStepize = 1;
const int servoCount = 6;

NLinkedList<Servo *> servoList = NLinkedList<Servo *>();

int servoPositions[servoCount];
int servoPositionsGoal[servoCount];
bool servosMovinig[servoCount];

long servoTimeBuffer;

void servo_init()
{
    servo0.attach(SERVO_0);
    servo1.attach(SERVO_1);
    servo2.attach(SERVO_2);
    servo3.attach(SERVO_3);
    servo4.attach(SERVO_4);
    servo5.attach(SERVO_5);

    servoList.add(&servo0);
    servoList.add(&servo1);
    servoList.add(&servo2);
    servoList.add(&servo3);
    servoList.add(&servo4);
    servoList.add(&servo5);
}

void servo_processLoop()
{

    for (int i = 0; i < servoList.size(); i++)
    {
        if (servoPositions[i] != servoPositionsGoal[i])
        {
            servosMovinig[i] = true;
            moveServo(i);
        }
        else
        {
            servosMovinig[i] = false;
        }
    }
};

void servo_processAction(int servoNo, int moveTo, int all, int stepSize)
{
    servoStepize = stepSize;
    servo_moveToPositionDegrees(servoNo, moveTo, all);
}

void servo_moveToPositionDegrees(int servoNo, int degrees, int all)
{
    pwmServoGoal = degrees;

    servoTimeBuffer = millis();
    servoPositionsGoal[servoNo] = degrees;
    if (all)
    {
        for (int i = 0; i < servoList.size(); i++)
        {
            servoPositionsGoal[i] = degrees;
        }
    }
}

void moveServo(int servoNumber)
{
    if (!servoTimeBufferComplete())
    {
        return;
    }
    int currentPos = servoPositions[servoNumber];
    int goalPos = servoPositionsGoal[servoNumber];

    if (currentPos > goalPos)
    {
        servoPositions[servoNumber] -= servoStepize;
        servoList.get(servoNumber)->write(currentPos - servoStepize);
    }
    else if (currentPos < goalPos)
    {
        servoPositions[servoNumber] += servoStepize;
        servoList.get(servoNumber)->write(currentPos + servoStepize);
    }
    else
    {
    }
}

bool servoTimeBufferComplete()
{
    if ((millis() - servoTimeBuffer) < SERVO_TIME_BUFFER)
    {
        return false;
    }
    else
    {
        servoTimeBuffer = millis();
        return true;
    }
}