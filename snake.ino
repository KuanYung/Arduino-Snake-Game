#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CLK 13
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_DC 10
#define TFT_CS 8
#define TFT_RST 9

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

#define BUTTON_LEFT 2
#define BUTTON_RIGHT 3
#define BUTTON_UP 4
#define BUTTON_DOWN 5

int screenWidth, screenHeight;
int gridSize = 10;   // 每格大小
int snakeLength = 5; // 初始蛇長
int score = 0;

struct SnakePart
{
  int x, y;
};

SnakePart snake[100];  // 最大蛇長
int direction = 0;     // 0=右，1=下，2=左，3=上
int lastDirection = 0; // 儲存最後有效方向

// 食物的位置
int foodX, foodY;

unsigned long lastMoveTime = 0;
unsigned long moveInterval = 150; // 初始速度設定為 300 毫秒
unsigned long lastScoreUpdate = 0;

void setup()
{
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);

  Serial.begin(9600);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);

  screenWidth = tft.width();
  screenHeight = tft.height();

  // 初始化蛇
  snake[0] = {screenWidth / 2, screenHeight / 2};
  for (int i = 1; i < snakeLength; i++)
  {
    snake[i] = {snake[i - 1].x - gridSize, snake[i - 1].y};
  }

  // 隨機生成食物
  spawnFood();

  displayScore();
}

void loop()
{
  handleInput();

  unsigned long currentMillis = millis();
  if (currentMillis - lastMoveTime >= moveInterval)
  {
    lastMoveTime = currentMillis;
    moveSnake();
    checkCollision();
    checkFoodCollision();

    // 根據分數增加遊戲速度
    if (score / 3 > lastScoreUpdate)
    {                              // 每3分增加速度
      lastScoreUpdate = score / 3; // 更新分數
      if (moveInterval > 50)
      {                     // 設置最低速度限制
        moveInterval -= 20; // 減少移動間隔以增加速度
      }
    }
  }
}

void handleInput()
{
  // 處理按鍵輸入，只有在按下按鍵時改變方向
  if (digitalRead(BUTTON_LEFT) == LOW && lastDirection != 0)
  {
    direction = 2; // Left
  }
  if (digitalRead(BUTTON_RIGHT) == LOW && lastDirection != 2)
  {
    direction = 0; // Right
  }
  if (digitalRead(BUTTON_UP) == LOW && lastDirection != 1)
  {
    direction = 3; // Up
  }
  if (digitalRead(BUTTON_DOWN) == LOW && lastDirection != 3)
  {
    direction = 1; // Down
  }
}

void drawSnake()
{
  for (int i = 0; i < snakeLength; i++)
  {
    tft.fillRect(snake[i].x, snake[i].y, gridSize, gridSize, ILI9341_BLUE);
  }
}

void moveSnake()
{
  // 更新 lastDirection，記錄實際移動方向
  lastDirection = direction;

  // 刪除蛇尾
  tft.fillRect(snake[snakeLength - 1].x, snake[snakeLength - 1].y, gridSize, gridSize, ILI9341_BLACK);

  // 移動蛇身
  for (int i = snakeLength - 1; i > 0; i--)
  {
    snake[i] = snake[i - 1];
  }

  // 根據當前方向更新蛇頭
  switch (direction)
  {
  case 0:
    snake[0].x += gridSize;
    break; // 向右移動
  case 1:
    snake[0].y += gridSize;
    break; // 向下移動
  case 2:
    snake[0].x -= gridSize;
    break; // 向左移動
  case 3:
    snake[0].y -= gridSize;
    break; // 向上移動
  }

  drawSnake(); // 繪製蛇
}

void checkCollision()
{
  // 撞牆邊邊界時會從對面出現
  if (snake[0].x < 0)
  {
    snake[0].x = screenWidth - gridSize;
  }
  if (snake[0].x >= screenWidth)
  {
    snake[0].x = 0;
  }
  if (snake[0].y < 0)
  {
    snake[0].y = screenHeight - gridSize;
  }
  if (snake[0].y >= screenHeight)
  {
    snake[0].y = 0;
  }

  // 撞到蛇身
  for (int i = 1; i < snakeLength; i++)
  {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
    {
      gameOver();
    }
  }
}

void checkFoodCollision()
{
  // 如果蛇頭碰到食物
  if (snake[0].x == foodX && snake[0].y == foodY)
  {
    score += 1;
    snakeLength++; // 增加蛇的長度

    // 隨機生成新的食物
    spawnFood();

    displayScore();
  }
}

void spawnFood()
{
  // 隨機生成食物位置，避免食物與蛇身重疊
  bool foodPlaced = false;
  while (!foodPlaced)
  {
    foodX = random(0, screenWidth / gridSize) * gridSize;
    foodY = random(0, screenHeight / gridSize) * gridSize;

    // 檢查食物是否與蛇身重疊
    foodPlaced = true;
    for (int i = 0; i < snakeLength; i++)
    {
      if (snake[i].x == foodX && snake[i].y == foodY)
      {
        foodPlaced = false;
        break;
      }
    }
  }

  // 顯示食物
  tft.fillRect(foodX, foodY, gridSize, gridSize, ILI9341_RED);
}

void gameOver()
{
  // 清除螢幕並顯示遊戲結束畫面
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(screenWidth / 4, screenHeight / 3);
  tft.print("GAME OVER");

  // 顯示分數
  tft.setCursor(screenWidth / 4, screenHeight / 2);
  tft.print("Score: ");
  tft.print(score);

  delay(3000); // 等待3秒鐘

  // 重置遊戲
  resetGame();
}

void resetGame()
{
  // 重置遊戲之前先清除螢幕
  tft.fillScreen(ILI9341_BLACK);

  // 當蛇撞自己時，重置遊戲
  score = 0;
  snakeLength = 5;
  direction = 0;
  lastDirection = 0; // 確保重置方向
  snake[0] = {screenWidth / 2, screenHeight / 2};
  for (int i = 1; i < snakeLength; i++)
  {
    snake[i] = {snake[i - 1].x - gridSize, snake[i - 1].y};
  }

  // 隨機生成食物
  spawnFood();

  // 重置遊戲速度
  moveInterval = 150; // 重置為初始速度

  displayScore();
}

void displayScore()
{
  tft.fillRect(0, 0, screenWidth, 10, ILI9341_BLACK); // 清除舊分數
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.print("Score: ");
  tft.print(score);
}
