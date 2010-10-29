#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/******************* BEGIN DECLARATIONS *******************/

/* Global Variables */
char board[32][32];				/* array storing board layout */
char ghostSPHome[32][32];		/* ghosts shortest-path home directions */
GLfloat wallHeight = 1.0;		/* heighte of maze walls */
GLuint walls;					/* wall display list */
int boardWidth = 0;				/* unit width of board */
int boardHeight = 0;			/* unit hight of board */
int pellets = 0;				/* number of pellets */
int superPellets = 0;			/* number of super-pellets */
int powerupTime = 7;			/* super-pellet powerup duration (secs) */
int playersLeft = 3;			/* number of remaining "men" */
int gameSpeed = 100;			/* speed of the movement function called */
int powerupTimeBase = 0;		/* start time of pacman power-up */
int ghostScoreMultiplier = 1;	/* score multiplier when eating a ghost */
int windowHeight = 400;			/* game window height */
int windowWidth = 400;			/* game window width */
int level = 0;					/* current level */
int numLevels = 0;				/* number of levels */
int tiltBoard = 0;				/* tilt board during play */
int gameStarted = 0;			/* set to 1 when playing */
int flashReady = 0;				/* flash READY!*/
int flashPointsX;				/* x position to position flashing text */
int flashPointsY;				/* y position to position flashing text */
int flashPointsScore;			/* points to flash at (x,y) */
int flashPoints = 0;			/* flash the points? */
int pacmanDefaultSpeed = 125;	/* default pacman speed */
int ghostDefaultSpeed = 125;	/* default ghost speed */
long score = 0;					/* player score */
long topScore = 10000;			/* top player score */
int newTopScore = 0;			/* did the player get a new top score? */

/* Array of level filenames - 32 max */
char levels[32][32];

/* Camera positioning and rotation angles */
GLfloat camera[] = {10.0, 1.0, 35.0}; //{13.0, 6.0, 45.0};
GLfloat theta[] = {-20.0, 0.0, 0.0};

/* Player structure attribute values */
enum directions {up, down, left, right};
enum status {dead, scared, normal, poweredUp};

/* Player structure; collection of attributes for pacman & ghosts */
struct player {
	int x, y;						/* x, y position */
	int startX, startY;				/* starting x, y position */
	int speed;						/* player's current movement speed */
	enum directions direction;		/* direction player is moving */
	enum directions nextDirection;	/* player's next direction */
	GLfloat colorf[3];				/* color of player */
	GLfloat size;					/* size of player */
	enum status state;				/* status of player */
} pacman, ghosts[4];

/* Lighting variables */
GLfloat l0_diffuse[] = {0.9, 0.9, 0.9, 1.0};
GLfloat l0_ambient[] = {0.2, 0.2, 0.2, 1.0};
GLfloat l0_specular[] = {0.7, 0.7, 0.7, 1.0};
GLfloat l0_pos[] = {5.0, 0.0, 5.0, 0.0};

/* Display functions */
void gameDisplay(void);
void gameOverDisplay(void);
void instrDisplay(void);
void killPacmanDisplay(void);
void levelUpDisplay(void);
void titleScreenDisplay(void);
void drawPacman(void);
void drawGhost(int);
void reshapeWindow(int, int);

/* Initialization functions */
void loadLevel(char*);
void initialize(void);

/* Input handler functions */
void keyInput(unsigned char, int, int);
void specialKeyInput(int, int, int);
void menu(int);

/* Player movement functions */
void movement(int);
void movePacman(int);
void moveGhosts(int);
void newGhostDirection(int);
void detectCollision(void);
int canMove(GLfloat, GLfloat, int);

/* Helper functions */
void colorGhost(int);
void colorGhosts(void);
void playerSetup(int);
int randomNum(int);
void disableFlashPoints(int);
void disableFlashReady(int);
void togglePause(void);
void toggleTilt(void);
void displayText(GLfloat, GLfloat, GLfloat, char*);

/******************** END DECLARATIONS ********************/

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("PacmanGL");

	printf("PacmanGL\n\n");
	printf("COMP3773 Project\n");
	printf("Shaun Kenny\n");
	printf("034616k@acadiau.ca\n\n");
	printf("Controls:\n  Movement: up, down, left, right arrows\n");
	printf("  p: Pause/unpause\n");
	printf("  t: Toggle board tilt during play\n");
	printf("  q: Quit\n");
	printf("Use the right mouse button for menus\n\n");

	/* Load board, initialize opengl, setup player/ghosts */
	initialize();

	/* Enable lighting */
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, l0_pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT, l0_ambient);
	glLightfv(GL_LIGHT0, GL_SPECULAR, l0_specular);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);

	/* Display title screen */
	glutDisplayFunc(titleScreenDisplay);

	glutMainLoop();
}

/* Function: gameDisplay
 * Parameters: none
 * Display function used when the player is playing the game.
 * Draws score, top score, the board, pacman and ghosts.  Sets
 * up right click menu with "Pause", "Toggle Tilt", "End Game" 
 * and "Quit" options.  If the gameStarted flag is set, re-registers
 * timer functions for movement.
 */
void gameDisplay() {
	int x, y, i;
	int digits = 1;
	char scoreStr[6];
	GLUquadricObj *sphere = gluNewQuadric();

	/* Build Menu */
	glutCreateMenu(menu);
	if (gameStarted)
		glutAddMenuEntry("Pause", 5);
	else
		glutAddMenuEntry("Un-pause", 5);
	glutAddMenuEntry("Toggle Tilt", 10);
	glutAddMenuEntry("End Game", 3);
	glutAddMenuEntry("Quit", 99);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	
	/* Position camera */
	gluLookAt(camera[0], camera[1], camera[2], 
			  boardWidth / 2.0, boardWidth / 2.0, 1.0, 
			  0.0, 1.0, 0.0); 

	/* Translate center of board to origin for rotation */
	glTranslatef(boardWidth / 2.0, boardHeight / 2.0, 0.0);
	glRotatef(theta[0], 1.0, 0.0, 0.0);
	glRotatef(theta[1], 0.0, 1.0, 0.0);
	glRotatef(theta[2], 0.0, 0.0, 1.0);
	glTranslatef(-(boardWidth / 2.0), -(boardHeight / 2.0), 0.0);

	/* Draw the floor */
	glBegin(GL_QUADS);
		glColor3f(0.5, 0.2, 0.2);
		glVertex2f(0.0, 0.0);
		glVertex2f(boardWidth, 0.0);
		glVertex2f(boardWidth, boardHeight);
		glVertex2f(0.0, boardHeight);
	glEnd();

	/* Draw the walls */
	glCallList(walls);

	/* Draw everything else */
	for (y = 0; y < boardHeight; y++) {
		for (x = 0; x < boardWidth; x++) {
			/* Pellet */
			if (board[y][x] == '.') {
				glDisable(GL_LIGHTING);
				glBegin(GL_POINTS);
					glColor3f((252.0 / 255.0), (182.0 / 255.0), (148.0 / 255.0));
					glVertex3f((GLfloat)x + 0.5, boardHeight - (GLfloat)y - 0.5, 0.5);
				glEnd();
				glEnable(GL_LIGHTING);
			}
			
			/* Super-pellet */
			if (board[y][x] == 'P') {
				glTranslatef(x + 0.5, ((boardHeight - y) - 0.5), 0.5);
				glColor3f((252.0 / 255.0), (182.0 / 255.0), (148.0 / 255.0));
				gluSphere(sphere, 0.3, 12, 12);
				glTranslatef(-(x + 0.5), -((boardHeight - y) - 0.5), -0.5);
			}
			
			/* Ghost door */
			if (board[y][x] == 'G') {
				glBegin(GL_QUADS);
					glColor3f((252.0 / 255.0), (182.0 / 255.0), (220.0 / 255.0));
					glVertex3f((GLfloat)x, boardHeight - (GLfloat)y - 0.5, 0.0);
					glVertex3f((GLfloat)x + 1.0, boardHeight - (GLfloat)y - 0.5, 0.0);
					glVertex3f((GLfloat)x + 1.0, boardHeight - (GLfloat)y - 0.5, wallHeight);
					glVertex3f((GLfloat)x, boardHeight - (GLfloat)y - 0.5, wallHeight);
				glEnd();
			}
		}
	}

	/* Draw pacman */
	glPushMatrix();
	glTranslatef(pacman.x + (pacman.size / 2.0), pacman.y - (pacman.size / 2.0), (pacman.size / 2.0));
	drawPacman();
	glPopMatrix();

	/* Draw the ghosts */
	for (i = 0; i < 4; i++) {
		glPushMatrix();
		glTranslatef(ghosts[i].x + (ghosts[i].size / 2.0), ghosts[i].y - (ghosts[i].size / 2.0), 0.0);
		drawGhost(i);
		glPopMatrix();
	}

	/* Draw players left */
	glPushMatrix();
	glTranslatef(boardWidth - 1.0, boardHeight + 2.0, 0.5);
	for (i = 0; i < playersLeft; i++) {
		glColor3fv(pacman.colorf);
		gluSphere(sphere, (pacman.size / 2.0) - 0.1, 10, 10);
		glTranslatef(-1.0, 0.0, 0.0);
	}
	glPopMatrix();

	/* Flash points if Pacman ate a ghost */
	if (flashPoints) {
		glDisable(GL_LIGHTING);
		glColor3f(1.0, 1.0, 1.0);
		displayText((GLfloat)flashPointsX, (GLfloat)flashPointsY, 2.5, itoa(flashPointsScore, scoreStr, 10));
		glEnable(GL_LIGHTING);
	}

	/* Flash READY! if just starting */
	if (flashReady > 0) {
		if (flashReady == 1 || flashReady == 3 || flashReady == 5) {
			glDisable(GL_LIGHTING);
			glColor3f(1.0, 1.0, 1.0);
			displayText(((GLfloat)boardWidth / 2.0) - 2.0, (GLfloat)boardHeight / 2.0, 2.0, "READY!");
			glEnable(GL_LIGHTING);
		}
	}

	/* Print score */
	glDisable(GL_LIGHTING);
	glColor3f(1.0, 1.0, 1.0);
	displayText(0.5, (GLfloat)boardHeight + 3.0, 1.0, "SCORE");
	displayText(0.5, (GLfloat)boardHeight + 1.5, 1.0, itoa(score, scoreStr,10));
	displayText(10.0, (GLfloat)boardHeight + 3.0, 1.0, "TOP SCORE");
	displayText(11.0, (GLfloat)boardHeight + 1.5, 1.0, itoa(topScore, scoreStr, 10));
	glEnable(GL_LIGHTING);

	gluDeleteQuadric(sphere);

	glFlush();
	glutSwapBuffers();
}

/* Function: gameOverDisplay
 * Parameters: none
 * Display function used when the player runs out of remaining
 * lives.
 */
void gameOverDisplay() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	/* Reset level, score */
	score = 0;
	level = 0;

	/* Build Menu */
	glutCreateMenu(menu);
	glutAddMenuEntry("Play Again", 1);
	glutAddMenuEntry("Title Screen", 3);
	glutAddMenuEntry("Quit", 99);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	
	/* Position camera */
	gluLookAt(camera[0], camera[1], camera[2], 
			  boardWidth / 2.0, boardWidth / 2.0, 1.0, 
			  0.0, 1.0, 0.0);

	glDisable(GL_LIGHTING);
	glColor3f(1.0, 1.0, 1.0);
	displayText(10.0, 14.0, 0.0, "GAME OVER!");
	if (newTopScore) {
		displayText(4.5, 11.0, 0.0, "You got a new high score!");	
	}
	glEnable(GL_LIGHTING);

	glFlush();
	glutSwapBuffers();
}

/* Function: instrDisplay
 * Parameters: none
 * Display function used when the player is at the instructions
 * screen.  Prints a lot of text and rotating player models.  Sets
 * up right click menu with "Play", "Title Screen" and "Quit"
 * options.  Continuously redisplays.
 */
void instrDisplay() {
	static int angle = 0;
	int i;
	GLUquadricObj *sphere = gluNewQuadric();

	/* Build Menu */
	glutCreateMenu(menu);
	glutAddMenuEntry("Play!", 1);
	glutAddMenuEntry("Title Screen", 3);
	glutAddMenuEntry("Quit", 99);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	/* Position camera */
	gluLookAt(camera[0], camera[1], camera[2], 
			  boardWidth / 2.0, boardWidth / 2.0, 1.0, 
			  0.0, 1.0, 0.0);

	/* Turn off lighting, print text */
	glColor3f(1.0, 1.0, 1.0);
	glDisable(GL_LIGHTING);
	displayText(9.0, 31.0, 0.0, "Instructions");
	displayText(11.0, 27.5, 0.0, "PLAYERS");
	displayText(2.0, 24.5, 0.0, "You:");
	displayText(15.0, 24.5, 0.0, "The Enemy:");
	displayText(11.0, 15.0, 0.0, "SCORING");
	displayText(13.0, 13.0, 0.0, "10 pts");
	displayText(13.0, 11.0, 0.0, "50 pts");
	displayText(10.0, 8.5, 0.0, "1st: 200 pts");
	displayText(10.0, 7.0, 0.0, "2nd: 400 pts");
	displayText(10.0, 5.5, 0.0, "3rd: 800 pts");
	displayText(10.0, 4.0, 0.0, "4th: 1600 pts");
	displayText(4.0, 1.0, 0.0, "Extra player every 20000 pts");
	displayText(6.0, -1.0, 0.0, "Use arrow keys to move");
	glEnable(GL_LIGHTING);

	/* You: pacman */
	glPushMatrix();
	glTranslatef(7.0, 25.0, 0.0);
	glRotatef(angle, 0.0, 1.0, 0.0);
	glScalef(2.0, 2.0, 2.0);
	drawPacman();
	glPopMatrix();
	
	/* Enemy: ghosts */
	glPushMatrix();
	glTranslatef(25.0, 24.5, 0.0);
	glScalef(2.0, 2.0, 2.0);
	for (i = 0; i < 4; i++) {
		glPushMatrix();
		glRotatef(angle, 0.0, 1.0, 0.0);
		glRotatef(-90, 1.0, 0.0, 0.0);
		drawGhost(i);
		glPopMatrix();
		glTranslatef(0.0, -1.0, 0.0);
	}
	glPopMatrix();

	/* Scoring: small pellet */
	glPushMatrix();
	glTranslatef(11.0, 13.5, 0.0);
	glRotatef(angle, 0.0, 1.0, 0.0);
	glColor3f((252.0 / 255.0), (182.0 / 255.0), (148.0 / 255.0));
	gluSphere(sphere, 0.25, 10, 10);
	glPopMatrix();

	/* Scoring: large pellet */
	glPushMatrix();
	glTranslatef(11.0, 11.5, 0.0);
	glRotatef(angle, 0.0, 1.0, 0.0);
	glColor3f((252.0 / 255.0), (182.0 / 255.0), (148.0 / 255.0));
	gluSphere(sphere, 0.5, 10, 10);
	glPopMatrix();

	/* Scoring: blue ghosts */
	glPushMatrix();
	glTranslatef(8.0, 8.0, 0.0);
	glScalef(2.0, 2.0, 2.0);
	glPushMatrix();
	glRotatef(angle, 0.0, 1.0, 0.0);
	glRotatef(-90, 1.0, 0.0, 0.0);
	ghosts[0].colorf[0] = 0.0;
	ghosts[0].colorf[1] = 0.0;
	ghosts[0].colorf[2] = 1.0;
	drawGhost(0);
	glPopMatrix();
	glTranslatef(0.0, -1.0, 0.0);
	colorGhosts();
	glPopMatrix();

	angle = (angle + 2) % 360;		/* rotation angle */

	gluDeleteQuadric(sphere);

	glFlush();
	glutSwapBuffers();

	glutPostRedisplay();
}

/* Function: killPacmanDisplay
 * Parameters: none
 * Display function used when the player is killed by a ghost.
 * Draws pacman at the current position and redisplays with a
 * static scale variable to shrink pacman off the screen.  When
 * this function is done drawing the death sequence, it resets
 * the player positions (pacman & ghosts) and registers the
 * next display function (gameDisplay if the player has
 * remaining lives or gameOverDisplay if the player has run out
 * of remaining lives).  Continually redisplays.
 */
void killPacmanDisplay() {
	static GLfloat scale = 1.0;
	static int count = 0;

	/* Stop movement */
	pacman.direction = -1;
	pacman.nextDirection = -1;
	gameStarted = 0;
	glutSpecialFunc(NULL);

	/* If tilt is enabled, return angle to starting position */
	if (tiltBoard) {
		theta[0] = -6.0;
		theta[1] = 0.0;
		theta[2] = 0.0;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	/* Position camera */
	gluLookAt(camera[0], camera[1], camera[2], 
			  boardWidth / 2.0, boardWidth / 2.0, 1.0, 
			  0.0, 1.0, 0.0);

	/* Draw pacman */
	glPushMatrix();
	glTranslatef(pacman.x + (pacman.size / 2.0), pacman.y - (pacman.size / 2.0), pacman.size / 2.0);
	glColor3fv(pacman.colorf);
	glScalef(scale, scale, scale);
	drawPacman();
	glPopMatrix();

	glFlush();
	glutSwapBuffers();

	if (count == 150) {
		/* Reset count, scale */
		count = 0;
		scale = 1.0;

		/* No more players? */
		if (playersLeft == 0) {
			glutDisplayFunc(gameOverDisplay);
			glutKeyboardFunc(NULL);
		} else {
			/* Reposition players */
			playerSetup(1);

			/* Disable point flashing */
			flashPoints = 0;

			/* Register callbacks */
			glutSpecialFunc(specialKeyInput);
			glutDisplayFunc(gameDisplay);
			flashReady = 5;
			glutTimerFunc(500, disableFlashReady, 1);
		}
	} else {
		if (scale > 0)
			scale -= 0.012;
		count++;
	}
	glutPostRedisplay();
}

/* Function: levelUpDisplay
 * Parameters: none
 * Display function used when the player is has completed a level.
 * Draws the board and pacman at the current position, and uses a
 * static rotation and scale varibale to spin the board around the 
 * origin and scale it down off the screen, while scaling pacman 
 * to the inverse of the scale variable to make him fly upward and 
 * through the screen.  Continuously redisplays.
 */
void levelUpDisplay() {
	static GLfloat scale = 1.0;
	static int angle = 0;
	static int count = 0;

	/* Stop movement */
	gameStarted = 0;
	glutSpecialFunc(NULL);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	
	/* Position camera */
	gluLookAt(camera[0], camera[1], camera[2], 
			  boardWidth / 2.0, boardWidth / 2.0, 1.0, 
			  0.0, 1.0, 0.0);

	/* Draw pacman */
	glPushMatrix();
	glTranslatef(pacman.x + (pacman.size / 2.0), pacman.y - (pacman.size / 2.0), pacman.size / 2.0);
	glScalef(1/scale, 1/scale, 1/scale);
	glColor3fv(pacman.colorf);
	drawPacman();
	glPopMatrix();

	/* Draw board */
	glPushMatrix();
	glRotatef(angle, 0.0, 0.0, -1.0);
	glScalef(scale, scale, scale);
	glCallList(walls);
	glPopMatrix();

	angle = (angle + 10) % 360;
	scale -= 0.025;
	count++;

	glFlush();
	glutSwapBuffers();

	if (count == 40) {
		/* Reset count, angle and scale */
		count = 0;
		scale = 1.0;
		angle = 0;

		/* Load next level */
		level = (level + 1) % numLevels;
		loadLevel(levels[level]);

		/* Speed up players */
		ghostDefaultSpeed -= 5;
		pacmanDefaultSpeed -= 3;

		/* Reduce power-up time */
		if (powerupTime > 3)
			powerupTime--;

		/* Register callbacks */
		glutDisplayFunc(gameDisplay);
		glutSpecialFunc(specialKeyInput);
		flashReady = 5;
		glutTimerFunc(500, disableFlashReady, 0);
	}
	glutPostRedisplay();
}

/* Function: titleScreenDisplay
 * Parameters: none
 * Display function used while the player is at the title
 * screen.  Sets up right click menu with "Play", "Instructions"
 * and "Quit" options.  Continuously redisplays.
 */
void titleScreenDisplay() {
	static int angle = 0;
	int digits = 1;
	char scoreStr[6];
	GLUquadricObj *sphere = gluNewQuadric();

	/* Build Menu */
	glutCreateMenu(menu);
	glutAddMenuEntry("Play!", 1);
	glutAddMenuEntry("Instructions", 2);
	glutAddMenuEntry("Quit", 99);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	/* Position camera */
	gluLookAt(camera[0], camera[1], camera[2], 
			  boardWidth / 2.0, boardWidth / 2.0, 1.0, 
			  0.0, 1.0, 0.0);

	/* Turn off lighting, print text */
	glColor3f(1.0, 1.0, 1.0);
	glDisable(GL_LIGHTING);
	displayText(9.5, 32.0, 0.0, "TOP SCORE:");
	displayText(11, 30.5, 0.0, itoa(topScore, scoreStr, 10));
	displayText(10.5, 27.0, 0.0, "PacmanGL");
	displayText(7.0, 1.0, 0.0, "Right Click For Menu");
	glEnable(GL_LIGHTING);

	/* Draw big pacman */
	glPushMatrix();
	glTranslatef((GLfloat)boardWidth / 2.0, (GLfloat)boardHeight / 2.0, 0.0);
	glRotatef(angle, 0.0, 1.0, 0.0);
	glColor3fv(pacman.colorf);
	gluSphere(sphere, 10.0, 24, 24);
	glPushMatrix();
	glTranslatef(-3.0, 1.5, 9.5);
	glColor3f(0.0, 0.0, 0.0);
	gluSphere(sphere, 1.0, 12, 12);
	glTranslatef(6.0, 0.0, 0.0);
	gluSphere(sphere, 1.0, 12, 12);
	glPopMatrix();
	glPopMatrix();

	angle = (angle + 2) % 360;		/* Rotation angle */

	gluDeleteQuadric(sphere);

	glFlush();
	glutSwapBuffers();

	glutPostRedisplay();
}

/* Function: drawPacman
 * Parameters: none
 * Draws pacman player on the screen at the current position.
 * Therefore, the co-ordinate system must be translated to the 
 * correct position and scaled if required before drawPacman 
 * is called.
 */
void drawPacman() {
	GLUquadricObj *sphere = gluNewQuadric();

	/* rotate pacman to face moving direction */
	switch (pacman.direction) {
	case left:
		glRotatef(-90, 0.0, 0.0, 1.0);
		break;
	case right:
		glRotatef(90, 0.0, 0.0, 1.0);
		break;
	case up:
		glRotatef(180, 0.0, 0.0, 1.0);
		break;
	}

	/* body */
	glColor3fv(pacman.colorf);
	gluSphere(sphere, (pacman.size / 2.0) - 0.1, 10, 10);
	
	/* left eye */
	glPushMatrix();
	glTranslatef(-0.2, -0.1, (pacman.size / 2.0) - 0.05);
	glColor3f(0.0, 0.0, 0.0);
	gluSphere(sphere, 0.1, 8, 8);
	glPopMatrix();
	
	/* right eye */
	glPushMatrix();
	glTranslatef(0.2, -0.1, (pacman.size / 2.0) - 0.05);
	gluSphere(sphere, 0.1, 8, 8);
	glPopMatrix();

	/* sunglasses bar */
	glPushMatrix();
	glTranslatef(-0.2, -0.05, (pacman.size / 2.0) - 0.05);
	glBegin(GL_LINES);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.4, 0.0, 0.0);
	glEnd();
	glPopMatrix();

	/* mouth */
	glPushMatrix();
	glTranslatef(-0.2, -0.3, 0.2);
	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.4, 0.0, 0.0);
	glEnd();
	glPopMatrix();

	gluDeleteQuadric(sphere);
}

/* Function: drawGhost
 * Parameters: int i
 * Draws the ghost specified by i on the screen at the current 
 * position.  Therefore, the co-ordinate system must be translated 
 * to the correct position and scaled if required before drawGhost 
 * is called.
 */
void drawGhost(int i) {
	GLUquadricObj *sphere = gluNewQuadric();
	
	if (ghosts[i].state != dead) {
		/* rotate ghost to face moving direction */
		switch (ghosts[i].direction) {
		case left:
			glRotatef(-90, 0.0, 0.0, 1.0);
			break;
		case right:
			glRotatef(90, 0.0, 0.0, 1.0);
			break;
		case up:
			glRotatef(180, 0.0, 0.0, 1.0);
			break;
		}

		/* body */
		glColor3fv(ghosts[i].colorf);
		gluCylinder(sphere, (ghosts[i].size / 2.0) - 0.1, (ghosts[i].size / 2.0) - 0.1, (ghosts[i].size / 2.0), 10, 10);

		/* head */
		glPushMatrix();
		glTranslatef(0.0, 0.0, (ghosts[i].size) / 2.0);
		gluSphere(sphere, (ghosts[i].size / 2.0) - 0.1, 10, 10);
	}
	
	/* Left eye */
	glPushMatrix();
	glTranslatef(-0.15, -0.2, -0.1);
	glColor3f(1.0, 1.0, 1.0);
	gluSphere(sphere, 0.2, 8, 8);
	glPopMatrix();
	/* Pupil */
	glPushMatrix();
	glTranslatef(-0.20, -0.3, -0.1);
	glColor3f(0.0, 0.0, 0.0);
	gluSphere(sphere, 0.1, 5, 5);
	glPopMatrix();

	/* Right eye */
	glPushMatrix();
	glTranslatef(0.15, -0.2, -0.1);
	glColor3f(1.0, 1.0, 1.0);
	gluSphere(sphere, 0.2, 8, 8);
	glPopMatrix();
	/* Pupil */
	glPushMatrix();
	glTranslatef(0.20, -0.3, -0.1);
	glColor3f(0.0, 0.0, 0.0);
	gluSphere(sphere, 0.1, 5, 5);
	glPopMatrix();

	/* If the ghost is alive, we pushed the matrix before 
	   drawing the head, so we have to pop it now */
	if (ghosts[i].state != dead)
		glPopMatrix();

	gluDeleteQuadric(sphere);
}

/* Function: reshapeWindow
 * Parameters: int w, int h
 * Callback function for when the window is resized
 */
void reshapeWindow(int w, int h) {
	/* Prevent problems when a window is too short */
	/* Windows cannot be resized to 0 width */
	if (h == 0)
		h = 1;
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (w > h)
		gluPerspective(45, w / h, 3, -3);
	else
		gluPerspective(45, h / w, 3, -3);

	glMatrixMode(GL_MODELVIEW);
}

/* Function: loadLevel
 * Parameters: char* filename
 * Loads the level specified by filename into the board array,
 * loads the ghost shortest path home board into the ghostSPHome
 * array, sets up initial player positions and generates a display
 * list to draw the maze walls */
void loadLevel(char* filename) {
	int i, j, x, y;
	int wallColorv[3];
	int readGhostboard = 0;
	char lineChar = '0';
	FILE* handle;

	/* Reset some global variables */
	boardHeight = 0;
	boardWidth = 0;
	pellets = 0;
	superPellets = 0;

	handle = fopen(filename, "r");
	if (handle == NULL) {
		printf("Could not open board file: %s\n", filename);
		exit(1);
	}

	/* Read level wall colours */
	fscanf(handle, "%d", &(wallColorv[0]));
	fscanf(handle, "%d", &(wallColorv[1]));
	fscanf(handle, "%d", &(wallColorv[2]));
	fscanf(handle, "%c", &lineChar);

	/* Scan file to determine board width and height */
	while (!feof(handle)) {
		fscanf(handle, "%c", &lineChar);
		/* If end of board layout */
		if (lineChar == 'Z')
			break;
		while(lineChar != '\n' && !feof(handle)) {
			if (boardHeight == 0) boardWidth++;
			fscanf(handle, "%c", &lineChar);
		}
		boardHeight++;
	}

	rewind(handle);

	/* Read level wall colours again */
	fscanf(handle, "%d", &(wallColorv[0]));
	fscanf(handle, "%d", &(wallColorv[1]));
	fscanf(handle, "%d", &(wallColorv[2]));
	fscanf(handle, "%c", &lineChar);

	/* Read level into array, set pacman/ghost starting positions */
	for (j = 0; j < boardHeight; j++) {
		for (i = 0; i < boardWidth; i++) {
			fscanf(handle, "%c", &lineChar);

			if (lineChar == 'S') {
				pacman.startY = boardHeight - j;
				pacman.startX = i;
			}

			if (lineChar == 'g') {
				ghosts[0].startY = boardHeight - j + 2;
				ghosts[0].startX = i + 1;
				ghosts[1].startY = boardHeight - j;
				ghosts[1].startX = i;
				ghosts[2].startY = boardHeight - j;
				ghosts[2].startX = i + 1;
				ghosts[3].startY = boardHeight - j;
				ghosts[3].startX = i + 2;
			}

			if (lineChar == '.')
				pellets++;

			if (lineChar == 'P')
				superPellets++;

			board[j][i] = lineChar;
		}

		fscanf(handle, "%c", &lineChar);	/* read EOL (\n) char */
	}

	fscanf(handle, "%c", &lineChar);		/* read Z char */
	fscanf(handle, "%c", &lineChar);		/* read EOL (\n) char */

	/* Read ghost shortest path home into array */
	for (j = 0; j < boardHeight; j++) {
		for (i = 0; i < boardWidth; i++) {
			fscanf(handle, "%c", &lineChar);
			ghostSPHome[j][i] = lineChar;
		}
		fscanf(handle, "%c", &lineChar);	/* read EOF (\n) char */
	}

	fclose(handle);

	/* Setup ghosts/pacman attributes */
	playerSetup(1);

	/* Generate display list for the walls */
	walls = glGenLists(1);
	glNewList(walls, GL_COMPILE);
		for (y = 0; y < boardHeight; y++) {
			for (x = 0; x < boardWidth; x++) {
				if (board[y][x] == '#') {
					glBegin(GL_QUADS);
						glColor3f(wallColorv[0] / 255.0, wallColorv[1] / 255.0, wallColorv[2] / 255.0);
						/* top */
						glNormal3i(0, 0, 1);
						glVertex3f((GLfloat)x, boardHeight - (GLfloat)y, wallHeight);
						glVertex3f((GLfloat)x + 1.0, boardHeight - (GLfloat)y, wallHeight);
						glVertex3f((GLfloat)x + 1.0, boardHeight - ((GLfloat)y + 1.0), wallHeight);
						glVertex3f((GLfloat)x, boardHeight - ((GLfloat)y + 1.0), wallHeight);
						/* left side */
						glNormal3i(-1, 0, 0);
						glVertex3f((GLfloat)x, boardHeight - (GLfloat)y, 0.0);
						glVertex3f((GLfloat)x, boardHeight - (GLfloat)y, wallHeight);
						glVertex3f((GLfloat)x, boardHeight - ((GLfloat)y + 1.0), wallHeight);
						glVertex3f((GLfloat)x, boardHeight - ((GLfloat)y + 1.0), 0.0);
						/* right side */
						glNormal3i(0, 1, 0);
						glVertex3f((GLfloat)x + 1.0, boardHeight - (GLfloat)y, 0.0);
						glVertex3f((GLfloat)x + 1.0, boardHeight - (GLfloat)y, wallHeight);
						glVertex3f((GLfloat)x + 1.0, boardHeight - ((GLfloat)y + 1.0), wallHeight);
						glVertex3f((GLfloat)x + 1.0, boardHeight - ((GLfloat)y + 1.0), 0.0);
						/* front */
						glNormal3i(0, -1, 0);
						glVertex3f((GLfloat)x, boardHeight - (GLfloat)y, 0.0);
						glVertex3f((GLfloat)x + 1.0, boardHeight - (GLfloat)y, 0.0);
						glVertex3f((GLfloat)x + 1.0, boardHeight - (GLfloat)y, wallHeight);
						glVertex3f((GLfloat)x, boardHeight - (GLfloat)y, wallHeight);
						/* back */
						glNormal3i(0, 1, 0);
						glVertex3f((GLfloat)x, boardHeight - ((GLfloat)y + 1.0), 0.0);
						glVertex3f((GLfloat)x + 1.0, boardHeight - ((GLfloat)y + 1.0), 0.0);
						glVertex3f((GLfloat)x + 1.0, boardHeight - ((GLfloat)y + 1.0), wallHeight);
						glVertex3f((GLfloat)x, boardHeight - ((GLfloat)y + 1.0), wallHeight);
					glEnd();
				}
			}
		}
	glEndList();
}

/* Function: initialize
 * Parameters: none
 * Reads list of levels into levels array, loads the first level,
 * sets up some OpenGL parameters */
void initialize() {
	FILE* handle;

	/* Seed random number generator */
	srand( (unsigned)time( NULL ) );

	/* Read level list */
	handle = fopen("levels.dat", "r");
	if (handle == NULL) {
		printf("Could not open board data file: levels.dat\n");
		exit(1);
	}
	while (!feof(handle)) {
		fscanf(handle, "%s", &levels[numLevels++]);
		if (numLevels == 32) break;		/* only load 32 levels */
	}
	fclose(handle);

	/* Load 1st level */
	loadLevel(levels[level]);

	/* Setup OpenGL */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, windowWidth / windowHeight, 3, -3);
	glMatrixMode(GL_MODELVIEW);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glPointSize(2.0);
}

/* Function: keyInput
 * Parameters: unsigned char key, int x, int y
 * Keyboard callback function to handle key presses
 */
void keyInput(unsigned char key, int x, int y) {
	switch (key) {
	case 'x':
		camera[0] += 0.5;
		break;
	case 'X':
		camera[0] -= 0.5;
		break;
	case 'y':
		camera[1] += 0.5;
		break;
	case 'Y':
		camera[1] -= 0.5;
		break;
	case 'z':
		camera[2] += 0.5;
		break;
	case 'Z':
		camera[2] -= 0.5;
		break;
	case 'l':
		printf("\nX: %f\nY: %f\nZ: %f\n", camera[0], camera[1], camera[2]);
		break;

	case 'p':	/* Pause / Un-pause */
		togglePause();
		break;

	case 't':	/* Toggle board tilt */
		toggleTilt();
		break;

	case 'w':
		glutDisplayFunc(levelUpDisplay);
		break;

	case 'q':	/* Quit */
		exit(1);
		break;
	}
}

/* Function: specialKeyInput
 * Parameters: int key, int x, int y
 * Keyboard callback function to handle arrow key presses
 */
void specialKeyInput(int key, int x, int y) {
	int i;

	/* Game has started! */
	if (gameStarted == 0)
		gameStarted = 1;

	switch (key) {
	case GLUT_KEY_UP:
		pacman.nextDirection = up;
		break;
	case GLUT_KEY_LEFT:
		pacman.nextDirection = left;
		break;
	case GLUT_KEY_DOWN:
		pacman.nextDirection = down;
		break;
	case GLUT_KEY_RIGHT:
		pacman.nextDirection = right;
		break;
	default:
		break;
	}

	/* If ghosts aren't moving, start moving them */
	for (i = 0; i < 4; i++) {
		if (ghosts[i].direction == -1) {
			ghosts[i].direction = left;
			ghosts[i].nextDirection = left;
		}
	}
}	

/* Function: menu
 * Parameters: int id
 * Right-click menu callback function.  Performs actions depending
 * on selected menu option.
 */
void menu(int id) {
	switch(id) {
	case 1:	/* Play game */
		loadLevel(levels[level]);
		flashReady = 5;
		playersLeft = 3;
		glutDisplayFunc(gameDisplay);
		glutKeyboardFunc(keyInput);
		glutSpecialFunc(specialKeyInput);
		glutTimerFunc(500, disableFlashReady, 0);
		glutPostRedisplay();
		break;
	case 2:	/* Instructions */
		glutDisplayFunc(instrDisplay);
		glutPostRedisplay();
		break;
	case 3:	/* End Game */
		gameStarted = 0;
		playerSetup(1);
		level = 0;
		score = 0;
		glutDisplayFunc(titleScreenDisplay);
		glutKeyboardFunc(NULL);
		glutSpecialFunc(NULL);
		glutPostRedisplay();
		break;
	case 5: /* Pause Game */
		togglePause();
		break;
	case 10:	/* Toggle Tilt */
		toggleTilt();
		break;
	case 99:	/* Quit */
		exit(0);
		break;
	}
}

/* Function: movement
 * Parameter: none
 * Handle general game movement and timing.  Keeps track of
 * how long Pacman has been powered-up and checks for Pacman/ghost
 * collision.  If the gameStarted flag is set, re-registers a 
 * timer function to call itself again.
 */
void movement(int a) {
	int time, i;
	static int toggleBlink = 0;

	/* Check if game has stopped; if so, return */
	if (gameStarted == 0)
		return;

	/* Check for ghost/pacman collision */
	detectCollision();

	/* Check powerup timer */
	if (powerupTimeBase != 0) {
		time = glutGet(GLUT_ELAPSED_TIME);
		if ((time - powerupTimeBase) >= powerupTime * 1000) {
			/* If powerup time has elapsed */
			powerupTimeBase = 0;
			toggleBlink = 0;
			ghostScoreMultiplier = 1;
			pacman.state = normal;
			pacman.speed = pacmanDefaultSpeed;
			for (i = 0; i < 4; i ++)
				if (ghosts[i].state == scared) {
					ghosts[i].state = normal;
					ghosts[i].speed = ghostDefaultSpeed;
				}
			colorGhosts();
		} else if ((time - powerupTimeBase) >= ((powerupTime * 1000) - 1000)) {
			/* If powerup time has 1 second left */
			if (toggleBlink == 0 || toggleBlink == 1) {
				toggleBlink++;
				colorGhosts();
			} else {
				toggleBlink++;
				if (toggleBlink == 3)
					toggleBlink = 0;
				for (i = 0; i < 4; i++) {
					if (ghosts[i].state == scared) {
						ghosts[i].colorf[0] = 0.0;
						ghosts[i].colorf[1] = 0.0;
						ghosts[i].colorf[2] = 1.0;
					}
				}
			}
		}
	}

	/* Check for ghost/pacman collision */
	detectCollision();

	/* Re-register timer func if game is in progress */
	if (gameStarted)
		glutTimerFunc(gameSpeed, movement, 0);

	glutPostRedisplay();
}

void movePacman(int a) {
	int i;
	static long extraScore = 20000;

	/* Check if game has stopped; if so, return */
	if (gameStarted == 0)
		return;

	/* Check for ghost/pacman collision */
	detectCollision();

	/* Change directions if possible */
	if (pacman.direction != pacman.nextDirection) {
		switch(pacman.nextDirection) {
		case left:
			if (canMove(pacman.x - 1.0, pacman.y, 5))
				pacman.direction = pacman.nextDirection;
			break;
		case up:
			if (canMove(pacman.x, pacman.y + 1.0, 5))
				pacman.direction = pacman.nextDirection;
			break;	
		case down:
			if (canMove(pacman.x, pacman.y - 1.0, 5))
				pacman.direction = pacman.nextDirection;
			break;
		case right:
			if (canMove(pacman.x + 1.0, pacman.y, 5))
				pacman.direction = pacman.nextDirection;
			break;
		}		
	}

	/* Actually move pacman */
	switch(pacman.direction) {
		case left:
			if (canMove(pacman.x - 1.0, pacman.y, 5)) {
				pacman.x -= 1;
			}
			break;
		case up:
			if (canMove(pacman.x, pacman.y + 1.0, 5)) {
				pacman.y += 1;
			}
			break;
		case down:
			if (canMove(pacman.x, pacman.y - 1.0, 5)) {
				pacman.y -= 1;
			}
			break;
		case right:
			if (canMove(pacman.x + 1.0, pacman.y, 5)) {
				pacman.x += 1;
			}
			break;
	}

	/* Check for ghost/pacman collision */
	detectCollision();

	if (tiltBoard) {
		theta[0] = pacman.y - (boardHeight / 2);
		theta[1] = -pacman.x + (boardWidth / 2);
	}

	/* Pacman eats a super-pellet */
	if (board[boardHeight - (int)pacman.y][(int)pacman.x] == 'P') {
		board[boardHeight - (int)pacman.y][(int)pacman.x] = '1';
		score += 50;
		superPellets--;
		pacman.state = poweredUp;
		if (pacman.speed == pacmanDefaultSpeed)
			pacman.speed -= 15;		/* speed up pacman */
		for (i = 0; i < 4; i++) {
			if (ghosts[i].state != dead) {
				ghosts[i].state = scared;
				if (ghosts[i].speed == ghostDefaultSpeed)
					ghosts[i].speed += 15;	/* slow down ghosts */
				ghosts[i].colorf[0] = 0.0;
				ghosts[i].colorf[1] = 0.0;
				ghosts[i].colorf[2] = 1.0;
				/* reverse direction */
				if (ghosts[i].direction == up) ghosts[i].direction = down;
				if (ghosts[i].direction == down) ghosts[i].direction = up;
				if (ghosts[i].direction == left) ghosts[i].direction = right;
				if (ghosts[i].direction == right) ghosts[i].direction = left;
			}
		}
		powerupTimeBase = glutGet(GLUT_ELAPSED_TIME);
	}

	/* Pacman eats a pellet */
	if (board[boardHeight - (int)pacman.y][(int)pacman.x] == '.') {
		board[boardHeight - (int)pacman.y][(int)pacman.x] = '1';
		score += 10;
		pellets--;
	}

	/* Pacman wraps around map */
	if (pacman.x == -1) pacman.x = boardWidth - 1;
	if (pacman.x == boardWidth) pacman.x = 0;
	if (pacman.y == 0) pacman.y = boardHeight;
	if (pacman.y == boardHeight + 1) pacman.y = 1;
 
	/* 20000 points - extra man */
	if (score >= extraScore) {
		extraScore += 20000;
		playersLeft++;
	}

	/* Update top score */
	if (score > topScore) {
		newTopScore = 1;
		topScore = score;
	}

	/* All pellets are gone */
	if (pellets == 0 && superPellets == 0) {
		glutDisplayFunc(levelUpDisplay);
		pacman.direction = -1;				/* stop pacman */
		for (i = 0; i < 4; i++)
			ghosts[i].direction = -1;		/* stop ghosts */
	}

	/* Check for ghost/pacman collision */
	detectCollision();

	/* Re-register movement timer func if game is going */
	if (gameStarted) {
		glutTimerFunc(pacman.speed, movePacman, 0);
	}

	glutPostRedisplay();
}

void moveGhosts(int a) {
	int i;
	char space;

	/* Check if game has stopped; if so, return */
	if (gameStarted == 0)
		return;

	/* Check for ghost/pacman collision */
	detectCollision();

	for (i = 0; i < 4; i++) {
		/* If the ghost is dead, return home */
		if (ghosts[i].state == dead) {
			space = ghostSPHome[boardHeight - (int)ghosts[i].y][(int)ghosts[i].x];
			switch (space) {
				case '9':
					ghosts[i].state = normal;
					ghosts[i].speed = ghostDefaultSpeed;
					ghosts[i].direction = up;
					colorGhost(i);
					break;
				case '0':
					ghosts[i].direction = up;
					break;
				case '1':
					ghosts[i].direction = down;
					break;
				case '2':
					ghosts[i].direction = left;
					break;
				case '3':
					ghosts[i].direction = right;
					break;
				default:
					break;

			}
		} else {
			/* Ghost isn't dead. Possibly change direction if at
			 * an intersection. */
			if (ghosts[i].direction != -1)	/* if ghosts are moving */
				newGhostDirection(i);
		}

		/* Check for ghost/pacman collision */
		detectCollision();

		/* Actually move ghost */
		switch (ghosts[i].direction) {
			case up:
				ghosts[i].y += 1.0;
				break;
			case down:
				ghosts[i].y -= 1.0;
				break;
			case left:
				ghosts[i].x -= 1.0;
				break;
			case right:
				ghosts[i].x += 1.0;
				break;
		}

		/* Ghosts wrap around map */
		if (ghosts[i].x == -1) ghosts[i].x = boardWidth - 1;
		if (ghosts[i].x == boardWidth) ghosts[i].x = 0;
		if (ghosts[i].y == 0) ghosts[i].y = boardHeight;
		if (ghosts[i].y == boardHeight + 1) ghosts[i].y = 1;
	}

	/* Check for ghost/pacman collision */
	detectCollision();

	/* Re-register movement timer func if game is going */
	if (gameStarted) {
		glutTimerFunc(ghosts[0].speed, moveGhosts, 0);
	}

	glutPostRedisplay();
}

/* Function: newGhostDirection
 * Parameters: int ghostNum
 * Randomly picks the direction the ghost specified by ghostNum
 * will move.  Ghosts will not do a 180 degree turn (turn around)
 * if another direction is possible.
 */
void newGhostDirection(int ghostNum) {
	int ghostMoves[4] = {0, 0, 0, 0};
	int count = 0;
	int i;
	int randNum;

	/* Check possible movement directions */
	if (canMove(ghosts[ghostNum].x, ghosts[ghostNum].y + 1.0, ghostNum)) {
		count++;
		ghostMoves[0] = 1;
	}
	if (canMove(ghosts[ghostNum].x, ghosts[ghostNum].y - 1.0, ghostNum)) {
		count++;
		ghostMoves[1] = 1;
	}
	if (canMove(ghosts[ghostNum].x - 1.0, ghosts[ghostNum].y, ghostNum)) {
		count++;
		ghostMoves[2] = 1;
	}
	if (canMove(ghosts[ghostNum].x + 1.0, ghosts[ghostNum].y, ghostNum)) {
		count++;
		ghostMoves[3] = 1;
	}

	/* Make sure the ghost doesn't just turn around if another
	   direction is possible */
	if (count > 1) {
		switch (ghosts[ghostNum].direction) {
		case up:
			ghostMoves[1] = 0;
			break;
		case down:
			ghostMoves[0] = 0;
			break;
		case left:
			ghostMoves[3] = 0;
			break;
		case right:
			ghostMoves[2] = 0;
			break;
		}
	}

	/* Pick a direction */
	if (count == 1) {
		/* Only one direction possible */
		for (i = 0; i < 4; i++) {
			if (ghostMoves[i] == 1) {
				ghosts[ghostNum].direction = i;
				ghosts[ghostNum].nextDirection = i;
				break;
			}
		}
	} else {
		/* Multiple directions possible - randomly pick one */
		while (1) {
			randNum = randomNum(4);
			if (ghostMoves[randNum] == 1) {
				ghosts[ghostNum].direction = randNum;
				ghosts[ghostNum].nextDirection = randNum;
				break;
			}
		}
	}
}

/* Function: detectCollision
 * Parameters: none
 * Detects whether or not pacman and a ghost have collided.  If
 * pacman is powered-up (he ate a super-pellet) then the ghost is
 * killed and it's state is changed.  Otherwise, pacman is killed
 * and the display function is registered to killPacmanDisplay.
 */
void detectCollision() {
	int i;
	int points;

	for (i = 0; i < 4; i++) {
		if (pacman.x == ghosts[i].x && pacman.y == ghosts[i].y) {
			if (ghosts[i].state == normal && pacman.state != dead) {
				pacman.state = dead;
				playersLeft--;
				/* reset ghost positions */
				playerSetup(0);
				/* change display func */
				glutDisplayFunc(killPacmanDisplay);
				glutPostRedisplay();
			} else if (pacman.state == poweredUp && ghosts[i].state == scared) {
				points = 200 * ghostScoreMultiplier;
				score += points;
				if (ghostScoreMultiplier < 8)
					ghostScoreMultiplier *= 2;
				ghosts[i].state = dead;
				ghosts[i].colorf[0] = 1.0;
				ghosts[i].colorf[1] = 1.0;
				ghosts[i].colorf[2] = 1.0;
				flashPointsX = ghosts[i].x;
				flashPointsY = ghosts[i].y;
				flashPointsScore = points;
				flashPoints++;
				glutTimerFunc(2000, disableFlashPoints, 0);
			}
		}
	}
}

/* Function: canMove
 * Parameters: GLfloat x, GLflost y, int playerNum
 * Function that checks if a player can move into the board
 * position specified by (x,y).  If there is a wall in the way,
 * canMove returns 0.  The playerNum parameter is required to
 * allow ghosts to walk through the ghost door, but not pacman.
 */
int canMove(GLfloat x, GLfloat y, int playerNum) {
	char nextSpace = board[boardHeight - (int)y][(int)x];

	switch (playerNum) {
	case 5:		/* Pacman */
		if (nextSpace != '#' && nextSpace != 'G') return 1;
		break;
	default:	/* Ghosts */
		if (nextSpace != '#') return 1;
		break;
	}

	return 0;
}

/* Function: colorGhost
 * Parameter: int i
 * Sets the color of the ghost specified by int i.
 */
void colorGhost(int i) {
	if (ghosts[i].state == normal) {
		switch (i) {
		case 0:
			ghosts[0].colorf[0] = (252.0 / 255.0);	/* Red ghost */
			ghosts[0].colorf[1] = (2.0 / 255.0);
			ghosts[0].colorf[2] = (4.0 / 255.0);
			break;
		case 1:
			ghosts[1].colorf[0] = (252.0 / 255.0);	/* Pink ghost */
			ghosts[1].colorf[1] = (182.0 / 255.0);
			ghosts[1].colorf[2] = (220.0 / 255.0);
			break;
		case 2:
			ghosts[2].colorf[0] = (4.0 / 255.0);	/* Blue ghost */
			ghosts[2].colorf[1] = (254.0 / 255.0);
			ghosts[2].colorf[2] = (220.0 / 255.0);
			break;
		case 3:
			ghosts[3].colorf[0] = (252.0 / 255.0);	/* Orange ghost */
			ghosts[3].colorf[1] = (182.0 / 255.0);
			ghosts[3].colorf[2] = (68.0 / 255.0);
			break;
		}
	} else if (ghosts[i].state == scared) {
		ghosts[i].colorf[0] = 255.0;
		ghosts[i].colorf[1] = 255.0;
		ghosts[i].colorf[2] = 255.0;
	}
}

/* Function: colorGhosts
 * Parameters: none
 * Calls the colorGhost function for each ghost
 */
void colorGhosts() {
	int i;

	for (i = 0; i < 4; i++)
		colorGhost(i);
}

/* Function: playerSetup
 * Parameters: int setPacmanPos
 * Sets the initial settings for all player attributes, such as
 * color, direction, position, etc.  If setPacmanPos == 0, the
 * function does not do this for the pacman player.
 */
void playerSetup(int setPacmanPos) {
	int i;

	/* Initialize ghosts */
	for (i = 0; i < 4; i++) {
		ghosts[i].x = ghosts[i].startX;
		ghosts[i].y = ghosts[i].startY;
		ghosts[i].state = normal;
		ghosts[i].speed = ghostDefaultSpeed;
		ghosts[i].direction = -1;
		ghosts[i].nextDirection = -1;
		ghosts[i].size = 1.0;
	}
	colorGhosts();

	/* Initialize pacman */
	if (setPacmanPos) {
		pacman.x = pacman.startX;
		pacman.y = pacman.startY;
		pacman.colorf[0] = (252.0 / 255.0);
		pacman.colorf[1] = (254.0 / 255.0);
		pacman.colorf[2] = (4.0 / 255.0);
		pacman.size = 1.0;
		pacman.state = normal;
		pacman.speed = pacmanDefaultSpeed;
		pacman.direction = -1;
		pacman.nextDirection = -1;
	}
}

/* Function: randomNum
 * Paramters: int max
 * Generates and returns random number between 0 and max
 */
int randomNum(int max) {
	return ((int)(max * rand()/(RAND_MAX + 1.0)));
}


/* Function: disableFlashPoints
 * Parameter: int i
 * Decrements the flashPoints counter.  If flashPoints is
 * greater than 1, points will be displayed on the screen when
 * the gameDisplay function is called next.
 */
void disableFlashPoints(int i) {
	flashPoints--;
}

/* Function: disableFlashReady
 * Parameter: int i
 * Decrements the flashReady counter.  If flashReady is
 * greater than 0 and odd, READY! is displayed in the middle of
 * the screen when the gameDisplay function is called next.
 */
void disableFlashReady(int i) {
	flashReady--;
	if (flashReady == 0) {
		/* Let's go! */
		gameStarted = 1;
		glutTimerFunc(gameSpeed, movement, 0);
		glutTimerFunc(pacman.speed, movePacman, 0);
		glutTimerFunc(ghosts[0].speed, moveGhosts, 0);
	} else {
		glutTimerFunc(500, disableFlashReady, 0);
	}
	glutPostRedisplay();
}

/* Function: togglePause
 * Parameters: none
 * Toggle whether or not the game is paused.  When the game
 * pauses, the function disables the special keyboard callback.
 */
void togglePause() {
	static int powerupTimeLeft;
	
	if (gameStarted == 1) {		/* Game is not paused, pause it */
		gameStarted = 0;
		glutSpecialFunc(NULL);	/* Disable arrow-key movement */

		/* Stop powerupTimer if it's going */
		if (powerupTimeBase != 0)
			powerupTimeLeft = ((powerupTime * 1000) - (glutGet(GLUT_ELAPSED_TIME) - powerupTimeBase));

	} else {					/* Game is paused, unpause it */
		gameStarted = 1;

		/* Start powerupTimeBase if it was stopped */
		if (powerupTimeLeft != 0) {
			powerupTimeBase = glutGet(GLUT_ELAPSED_TIME) - ((powerupTime * 1000) - powerupTimeLeft);
			powerupTimeLeft = 0;
		}

		glutSpecialFunc(specialKeyInput);
		glutTimerFunc(gameSpeed, movement, 0);
		glutTimerFunc(pacman.speed, movePacman, 0);
		glutTimerFunc(ghosts[0].speed, moveGhosts, 0);
	}
}

/* Function: toggleTilt
 * Parameters: none
 * Toggle whether or not board tilting is enabled.  When tilting
 * is enabled, the board tilts as the player moves.
 */
void toggleTilt() {
	tiltBoard = (tiltBoard + 1) % 2;

	/* If tilt is disabled, return angle to original position */
	if (tiltBoard == 0) {
		theta[0] = -20.0;
		theta[1] = 0.0;
		theta[2] = 0.0;
	}
}

/* Function: displayText
 * Parameters: GLfloat x, GLfloat y, char* string
 * Sets the raster position to x,y,z and prints string.
 */
void displayText(GLfloat x, GLfloat y, GLfloat z, char* string) {
	int i;

	if (z == 0.0)
		glRasterPos2f(x, y);
	else
		glRasterPos3f(x, y, z);
	for (i = 0; i < (int)strlen(string); i++)
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, string[i]);
}