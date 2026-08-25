// LED INDICATOR ========================
#define LED_INDICATOR_PIN 2 // D4 on NodeMCU, built-in LED
bool ledIndicator = false;
int ledStart = 0;
long ledDuration = 300; // 300ms
long ledDelay = 5000;   // 5s
enum LedState
{
    DISCONNECTED,
    CONNECTED
};
LedState currLedState = DISCONNECTED;
void startLedIndicator()
{
    digitalWrite(LED_INDICATOR_PIN, LOW);
    ledIndicator = true;
    ledStart = millis();
}
void stopLedIndicator()
{
    digitalWrite(LED_INDICATOR_PIN, HIGH);
    ledIndicator = false;
    ledStart = millis();
}
void initLedIndicator()
{
    pinMode(LED_INDICATOR_PIN, OUTPUT);
    startLedIndicator();
}
void updateLedIndicator()
{
    switch (currLedState)
    {
    case DISCONNECTED:
        if (millis() - ledStart >= 200)
        {
            digitalWrite(LED_INDICATOR_PIN, !digitalRead(LED_INDICATOR_PIN));
            ledStart = millis();
        }
        break;

    default:
        if (ledIndicator && millis() - ledStart >= ledDuration)
        {
            stopLedIndicator();
        }

        if (!ledIndicator && millis() - ledStart >= ledDelay)
        {
            startLedIndicator();
        }
        break;
    }
}
// LED INDICATOR ========================
