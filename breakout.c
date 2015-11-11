// standard libraries
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Stanford Portable Library
#include <spl/gevents.h>
#include <spl/gobjects.h>
#include <spl/gwindow.h>

// height and width of game's window in pixels
#define HEIGHT 600
#define WIDTH 400

// height and width of the paddle
#define P_HEIGHT 20
#define P_WIDTH 120

// height and width of a brick
#define B_HEIGHT 20
#define B_WIDTH 36

// space between bricks
#define SPACE 3

// number of rows of bricks
#define ROWS 5

// number of columns of bricks
#define COLS 10

// radius of ball in pixels
#define RADIUS 10

// lives
#define LIVES 3

// prototypes
void initBricks(GWindow window);
GOval initBall(GWindow window);
GRect initPaddle(GWindow window);
GLabel initScoreboard(GWindow window);
GLabel initLives(GWindow window);
void updateScoreboard(GWindow window, GLabel label, int points);
void updateLives(GWindow window, GLabel label, int points);
bool removeBrick(GWindow win, GObject brick, GLabel l, int *score, int *bricks);
GObject detectCollision(GWindow window, GOval ball);

int main(int argc, char** argv) {
  // seed pseudorandom number generator
  srand48(time(NULL));

  // instantiate window
  GWindow window = newGWindow(WIDTH, HEIGHT);

  // instantiate bricks
  initBricks(window);

  // instantiate ball, centered in middle of window
  GOval ball = initBall(window);

  // instantiate paddle, centered at bottom of window
  GRect paddle = initPaddle(window);

  // instantiate scoreboard, positioned at the upper part of the window
  GLabel scoreLabel = initScoreboard(window);

  // intantiate lives, positioned at the upper part of the window
  GLabel livesLabel = initLives(window);

  // number of bricks initially
  int bricks = COLS * ROWS;

  // number of lives initially
  int lives = LIVES;

  // number of points initially
  int score = 0;

  // check if super mode, ultimate mode or god mode are enabled
  // note: super mode increases velocity of the ball
  // note: in god mode, the paddle moves perfectly on its own
  bool superMode = false;
  bool ultimateMode = false;
  bool godMode = false;
  bool laserMode = false;

  int i;
  for (i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "GOD") == 0)
      godMode = true;
    if (strcmp(argv[i], "SUPER") == 0)
      superMode = true;
    if (strcmp(argv[i], "ULTIMATE") == 0)
      ultimateMode = true;
    if (strcmp(argv[i], "LASER") == 0)
      laserMode = true;
  }

  // velocity of ball
  double velocityX = drand48() + 2, velocityY = drand48() + 2;

  if (superMode == true) {
    velocityX = -5;
    velocityY = -5;
  }

  if (ultimateMode == true) {
    velocityX = -10;
    velocityY = -10;
  }

  // wait for click before starting
  waitForClick();

  // keep playing until game over
  while (lives > 0 && bricks > 0) {
    while (true) {
      // check for mouse event
      GEvent event = getNextEvent(MOUSE_EVENT);

      if (godMode == true) {
        // ensure that the paddle follows the ball
        int x = getX(ball) - getWidth(paddle) / 2;
        int y = getY(paddle);
        setLocation(paddle, x, y);
      }

      // if heard a mouse event
      if (event != NULL) {
        // if event was a movement event
        if (godMode == false && getEventType(event) == MOUSE_MOVED) {
          // ensure that the paddle follows the cursor
          int x = getX(event) - getWidth(paddle) / 2;
          int y = getY(paddle);
          setLocation(paddle, x, y);
        }

        // fire laser if laser mode is on
        if (laserMode == true && getEventType(event) == MOUSE_CLICKED) {
          // get first brick laser will collide with
          int pos = 0, i;
          for (i = getY(paddle) - 1; i >= 0; --i) {
            // check if there's an object at the given point
            GObject obj = getGObjectAt(window, getX(paddle) +
            getWidth(paddle) / 2, i);

            // check if the object is a brick
            if (obj != NULL && strcmp(getType(obj), "GRect") == 0) {
              pos = i;
              if (removeBrick(window, obj, scoreLabel, &score, &bricks))
                godMode = true;
              break;
            }
          }

          // instantaite a laser beam
          GLine laser = newGLine(getX(paddle) + getWidth(paddle) / 2,
          getY(paddle) + getHeight(paddle) / 2,
          getX(paddle) + getWidth(paddle) / 2, pos);

          // show laser
          add(window, laser);

          // pause laser
          pause(15);

          // remove laser
          removeGWindow(window, laser);
        }
      }

      // move the ball
      move(ball, velocityX, velocityY);

      // bounce on edges
      if (getX(ball) <= 0 || getX(ball) + RADIUS * 2 >= WIDTH)
        velocityX = -velocityX;

      if (getY(ball) <= 0)
        velocityY = -velocityY;

      // detect collision with objects
      GObject object = detectCollision(window, ball);

      if (object == paddle) {
        velocityY = -velocityY;
      } else if (object != NULL && strcmp(getType(object), "GRect") == 0) {
        // remove brick after collision
        if (removeBrick(window, object, scoreLabel, &score, &bricks))
          godMode = true;

        // change direction of ball
        velocityY = -velocityY;

        // increase velocity
        if (velocityX > 0)
          velocityX += 0.1;
        if (velocityY > 0)
          velocityY += 0.1;
        if (velocityX < 0)
          velocityX -= 0.1;
        if (velocityY < 0)
          velocityY -= 0.1;

        // shrink paddle
        setBounds(paddle, getX(paddle), getY(paddle),
        getWidth(paddle) - 1, getHeight(paddle));
      }

      // if passed the paddle, end game
      if (getY(ball) + RADIUS * 2 >= HEIGHT) {
        // reset ball location
        setLocation(ball, WIDTH / 2 - RADIUS, HEIGHT / 2 - RADIUS);

        // update lives
        --lives;
        updateLives(window, livesLabel, lives);

        // if no more lives, end game
        if (lives == 0) {
          GLabel gameOver = newGLabel("");
          setFont(gameOver, "SansSerif-32");
          setLabel(gameOver, "Game Over");
          setLocation(gameOver, WIDTH / 2 - getWidth(gameOver) / 2,
          HEIGHT / 2 - getHeight(ball));
          add(window, gameOver);
          break;
        }

        waitForClick();
      }

      pause(10);
    }
  }

  // wait for click before exiting
  waitForClick();

  // game over
  closeGWindow(window);
  return 0;
}

/**
 * Initializes window with a grid of bricks.
 */
void initBricks(GWindow window) {
  const string COLORS[] = { "BLUE", "RED", "YELLOW", "RED", "BLUE" };
  int i, j;
  for (i = 0; i < COLS; ++i) {
    for (j = 2; j < ROWS + 2; ++j) {
      // instantiate a brick and add it to the window
      GRect brick = newGRect(2 * SPACE + i * (B_WIDTH + SPACE),
          j * (B_HEIGHT + SPACE), B_WIDTH, B_HEIGHT);
      setColor(brick, COLORS[j - 2]);
      setFilled(brick, true);
      add(window, brick);
    }
  }
}

/**
 * Instantiates ball in center of window.  Returns ball.
 */
GOval initBall(GWindow window) {
  // position the ball at the centre of the window
  int x = WIDTH / 2 - RADIUS;
  int y = HEIGHT / 2 - RADIUS;
  GOval ball = newGOval(x, y, RADIUS * 2, RADIUS * 2);
  setFilled(ball, true);
  add(window, ball);
  return ball;
}

/**
 * Instantiates paddle in bottom-middle of window.
 */
GRect initPaddle(GWindow window) {
  // position the padddle at the bottom-middle of the window
  int x = WIDTH / 2 - P_WIDTH / 2;
  int y = HEIGHT - P_HEIGHT - 50;

  // instintiate the paddle
  GRect paddle = newGRect(x, y, P_WIDTH, P_HEIGHT);

  // fill the paddle with the color
  setFilled(paddle, true);

  // add the paddle to the window
  add(window, paddle);

  // return the paddle
  return paddle;
}

/**
 * Instantiates, configures, and returns label for scoreboard.
 */
GLabel initScoreboard(GWindow window) {
  // use to set labels
  char s[12];

  // instantiate a label for the word "score"
  GLabel scoreLabel = newGLabel("scoreLabel");
  setFont(scoreLabel, "SansSerif-18");
  setLabel(scoreLabel, "Score: ");
  setLocation(scoreLabel, 10, 30);
  add(window, scoreLabel);

  // instantaite label for score
  GLabel score = newGLabel("livesLabel");
  setFont(score, "SansSerif-18");
  sprintf(s, "%d", 0);
  setLabel(score, s);
  setLocation(score, 10 + 70, 30);
  add(window, score);

  return score;
}

/**
 * Instantiates, configures, and returns label for lives.
 */

GLabel initLives(GWindow window) {
  // use to set labels
  char s[12];

  // instantaite a label for the word "lives"
  GLabel livesLabel = newGLabel("livesLabel");
  setFont(livesLabel, "SansSerif-18");
  setLabel(livesLabel, "Lives: ");
  setLocation(livesLabel, 200, 30);
  add(window, livesLabel);

  // instantiate label for lives
  GLabel lives = newGLabel("livesLabel");
  setFont(lives, "SansSerif-18");
  sprintf(s, "%d", LIVES);
  setLabel(lives, s);
  setLocation(lives, 200 + 70, 30);
  add(window, lives);

  return lives;
}

/**
 * Updates scoreboard's label, keeping it at its position.
 */
void updateScoreboard(GWindow window, GLabel label, int points) {
  // update label
  char s[12];
  sprintf(s, "%d", points);
  setLabel(label, s);

  // fix the location
  setLocation(label, 10 + 70, 30);
}

/**
 * Updates lives' label, keeping it at its position.
 */
void updateLives(GWindow window, GLabel label, int points) {
  // update label
  char s[12];
  sprintf(s, "%d", points);
  setLabel(label, s);

  // fix the location
  setLocation(label, 200 + 70, 30);
}

/**
 * Removes the passed brick from the window, updates the score
 * with the new score passed, shrinks the paddle, and increased
 * the velocity of the ball.
 * Returns true if no bricks left.
 */
bool removeBrick(GWindow win, GObject brick, GLabel l, int *score,
    int *bricks) {
  ++(*score);
  --(*bricks);
  removeGWindow(win, brick);
  updateScoreboard(win, l, *score);

  // if no more bricks, end game
  if (*bricks == 0) {
    GLabel gameOver = newGLabel("");
    setFont(gameOver, "SansSerif-32");
    setLabel(gameOver, "Congratulations!");
    setLocation(gameOver, WIDTH / 2 - getWidth(gameOver) / 2,
    HEIGHT / 2 - B_HEIGHT);
    add(win, gameOver);
    return 1;
  }
  return 0;
}

/**
 * Detects whether ball has collided with some object in window
 * by checking the four corners of its bounding box (which are
 * outside the ball's GOval, and so the ball can't collide with
 * itself).  Returns object if so, else NULL.
 */
GObject detectCollision(GWindow window, GOval ball) {
  // ball's location
  double x = getX(ball);
  double y = getY(ball);

  // for checking for collisions
  GObject object;

  // check for collision at ball's top-left corner
  object = getGObjectAt(window, x, y);
  if (object != NULL) {
    return object;
  }

  // check for collision at ball's top-right corner
  object = getGObjectAt(window, x + 2 * RADIUS, y);
  if (object != NULL) {
    return object;
  }

  // check for collision at ball's bottom-left corner
  object = getGObjectAt(window, x, y + 2 * RADIUS);
  if (object != NULL) {
    return object;
  }

  // check for collision at ball's bottom-right corner
  object = getGObjectAt(window, x + 2 * RADIUS, y + 2 * RADIUS);
  if (object != NULL) {
    return object;
  }

  // no collision
  return NULL;
}
