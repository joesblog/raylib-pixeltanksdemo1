/*******************************************************************************************
*
*   raylib game template
*
*   <Game title>
*   <Game description>
*
*   This game has been created using raylib (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2021 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include "screens.h"    // NOTE: Declares global (extern) variables and screens functions

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif
#include <math.h>
#include <utils.h>
enum playerAction { WALKING = 1, FALLING = 2, ASCENDING = 3, STANDING = 4, DEAD = 5 };

typedef struct Ball {
	Vector2 position;
	Vector2 speed;
	int radius;
	bool active;
} Ball;

typedef struct Player {
	Vector2 position;
	Vector2 size;

	Vector2 movement;

	Vector2 aimingPoint;
	int aimingAngle;
	int aimingPower;

	Vector2 previousPoint;
	int previousAngle;
	int previousPower;

	Vector2 impactPoint;

	bool isLeftTeam;                // This player belongs to the left or to the right team
	bool isPlayer;                  // If is a player or an AI
	bool isAlive;

	playerAction paction;
	int Ascended;
	int Fallen;
	int TrueFallen;
} Player;

#define GRAVITY                       9.81f
#define DELTA_FPS                        60
const int MAXFALLDISTANCE = 62;

int fIteration = 0;
int fClockFrame = 0;

Vector2 camStart = { 342,388 };
Camera2D mainCam = { 0 };

Image imgBg;
Texture texBg;
int* maskBg = nullptr;

Image imgCn;
Texture texCn;

Image imgBomb;
int* maskBomb = nullptr;

int Width = 0;
int Height = 0;
int Size = 0;

int bombWidth = 0;
int bombHeight = 0;
int bombSize = 0;


int imgInvalid = 0;

Vector2 cannonPos = { 334,288 };
float cannonAngle = 0;
Vector2 prevPos = { 0,0 };


static Ball ball = { 0 };
static Player player = { 0 };
static bool ballOnAir = false;
void setup();
void setupBGMask();
void setupBombMask();

void render();

void CheckAndUpdateTexture();
bool updatePlayer(Vector2& thisPos);
void handlelogic(Vector2& thisPos);
void handleInput(Vector2& thisPos);
void handleWalking();
void handleFalling();
void handleAscending();
void handleStanding();
void handlePlayerMovt();
void transitionState(enum playerAction newState, bool turnAround = false);
void TurnAround() { player.movement.x = -player.movement.x; }
bool updateBall();
int  main(void);

void cutPx(int x, int y);
void cutBombMask(int cx, int cy);
int HasPixelAt(int x, int y);

int findGroundPixel(int x, int y);


int  main(void)
{
	setup();

#if defined PLATFORM_WEB
	emscripten_set_main_loop(render, 60, 1);
#else
	while (!WindowShouldClose())
		render();
#endif 
}

void setup()
{
	InitWindow(1024, 768, "Cannons");
	SetTargetFPS(60);
	mainCam.offset = { 0,0 };
	mainCam.target = { 0,0 };
	mainCam.zoom = 1;
	mainCam.rotation = 0;


	imgBg = LoadImage("resources/demoBg.png");
	ImageFormat(&imgBg, PixelFormat::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
	texBg = LoadTextureFromImage(imgBg);
	Width = imgBg.width;
	Height = imgBg.height;
	Size = Width * Height;

	setupBGMask();

	imgCn = LoadImage("resources/cannon.png");
	ImageFormat(&imgCn, PixelFormat::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);


	imgBomb = LoadImage("resources/bombmask.png");

	bombHeight = imgBomb.height;
	bombWidth = imgBomb.width;
	bombSize = bombHeight * bombWidth;
	setupBombMask();

	UnloadImage(imgBomb);


	texCn = LoadTextureFromImage(imgCn);
	UnloadImage(imgCn);



	player.isAlive = true;

	player.position = cannonPos;

	// Now there is no AI
	player.isPlayer = true;


	player.size = (Vector2){32, 32};


	// Set statistics to 0
	player.aimingPoint = player.position;
	player.previousAngle = 0;
	player.previousPower = 0;
	player.previousPoint = player.position;
	player.aimingAngle = 0;
	player.aimingPower = 0;

	player.impactPoint = { -100, -100 };

	player.paction = STANDING;
	player.Ascended = 0;
	player.Fallen = 0;
	player.TrueFallen = 0;
	transitionState(WALKING);
	ball.radius = 10;
	ballOnAir = false;
	ball.active = false;



}
void CheckAndUpdateTexture()
{

	if (imgInvalid)
	{
		Color* pxImg = LoadImageColors(imgBg);


		UpdateTexture(texBg, pxImg);

		//can optimize this for just localised changes
		for (int y = 0; y < Height; y++)
		{

			for (int x = 0; x < Width; x++)
			{

				int ix = y * Width + x;

				if (ix >= Size || ix <= 0) continue;

				if (maskBg[ix] == 0)
				{
					pxImg[ix] = BLANK;


				}

			}
		}

		UpdateTexture(texBg, pxImg);
		UnloadImage(imgBg);
		imgBg = LoadImageFromTexture(texBg);

		UnloadImageColors(pxImg);


	}
	imgInvalid = 0;
}
void render()
{


	Vector2 thisPos = GetMousePosition();

	handleInput(thisPos);
	handlelogic(thisPos);
	CheckAndUpdateTexture();
	BeginDrawing();
	BeginMode2D(mainCam);
	ClearBackground({ 0,0,52,255 });

	DrawTexture(texBg, 0, 0, WHITE);

	DrawTexture(texCn, player.position.x - (texCn.width / 2), player.position.y - (texCn.height - 4), WHITE);
	//DrawCircle(player.position.x, player.position.y, 10, RED);
	//const char* txt = TextFormat("x%f, y%f", cannonPos.x, cannonPos.y);
	//DrawText(txt, 10, 10, 14, WHITE);

	//DrawText(TextFormat("%i", player.paction), 10, 60, 18, WHITE);



	if (ball.active) DrawCircle(ball.position.x, ball.position.y, ball.radius, MAROON);
	if (!ballOnAir)
		DrawTriangle(
			{ player.position.x - player.size.x / 2, player.position.y - player.size.y / 4 },
			{ player.position.x + player.size.x * 2, player.position.y + player.size.y / 4 },
			player.aimingPoint, { 255,255,255,100 });
	EndMode2D();
	EndDrawing();


}


bool updatePlayer(Vector2& thisPos)
{
	handlePlayerMovt();

	if (thisPos.y <= player.position.y)
	{
		player.aimingPower = sqrt(pow(player.position.x - GetMousePosition().x, 2) + pow(player.position.y - GetMousePosition().y, 2));

		player.aimingAngle = asin((player.position.y - GetMousePosition().y) / player.aimingPower) * RAD2DEG;

		player.aimingPoint = GetMousePosition();

		if (GetMousePosition().x < player.position.x)
		{

			player.isLeftTeam = true;
		}
		else {
			player.isLeftTeam = false;
}
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			player.previousPoint = player.aimingPoint;
			player.previousPower = player.aimingPower;
			player.previousAngle = player.aimingAngle;
			ball.position = player.position;
			transitionState(STANDING);
			return true;
		}
	}
	else
	{
		player.aimingPoint = player.position;
		player.aimingPower = 0;
		player.aimingAngle = 0;
	}


	//implement some terrain tracking a-la Lemmings.



	return false;
}


void handlePlayerMovt()
{


	fIteration++;
	if (fIteration < 2)
		return;

	fIteration = 0;

	if (player.position.y >= GetScreenHeight())
		transitionState(DEAD);


	switch (player.paction)
	{
	case STANDING:
	{
		handleStanding();
		break;
	}
	case WALKING:
	{
		handleWalking();
		break;
	}
	case ASCENDING:
	{
		handleAscending();
		break;
	}
	case FALLING:
	{
		handleFalling();
		break;
	}


	default:
		handleStanding();
		break;
	}
}
bool updateBall()
{
	if (!ball.active)
	{
		ball.speed.x = cos(player.previousAngle * DEG2RAD) * player.previousPower * 3 / DELTA_FPS;
		ball.speed.y = -sin(player.previousAngle * DEG2RAD) * player.previousPower * 3 / DELTA_FPS;
		ball.active = true;
		if (player.isLeftTeam) ball.speed.x = -ball.speed.x;

	}

	ball.position.x += ball.speed.x;
	ball.position.y += ball.speed.y;
	ball.speed.y += GRAVITY / DELTA_FPS;


	if (ball.position.x + ball.radius < 0 || ball.position.y >= GetScreenHeight())
	{
		return true;
	}

	else if (ball.position.x - ball.radius > GetScreenWidth())
	{
		return true;

	}



	//check for terrain collision using pixel color
	//this is different from the example

	Color c = GetImageColor(imgBg, ball.position.x + ball.radius, ball.position.y);
	if (c.a > 0) //simple alpha check value
	{
		return true;


	}

	return false;
}
void handlelogic(Vector2& thisPos)
{
	/*fIteration++;
	fClockFrame++;
	if (fClockFrame == 17) //aprox 1 second
	{
		fClockFrame = 0;
	}*/


	if (!ballOnAir) ballOnAir = updatePlayer(thisPos);
	else {
		if (updateBall())
		{

			ball.active = false;
			ballOnAir = false;
			cutBombMask(ball.position.x, ball.position.y + bombHeight / 2);
		}
	}

}
void handleInput(Vector2& thisPos)
{
	Vector2 delta = Vector2Subtract(prevPos, thisPos);

	//player.movement.x = 0;
//	player.paction = STANDING;
	if (IsKeyPressed(KEY_PAGE_UP))
	{
		mainCam.zoom += 1;
	}
	else if (IsKeyPressed(KEY_PAGE_DOWN))
	{

		mainCam.zoom -= 1;
	}

	if (IsKeyPressed(KEY_RIGHT))
	{
		player.paction = WALKING;

		player.movement.x = 1;
	}
	else if (IsKeyPressed(KEY_LEFT))
	{
		player.paction = WALKING;

		player.movement.x = -1;

	}

}

void transitionState(playerAction newState, bool turnAround)
{
	Vector2& pos = player.position;
	Vector2& mov = player.movement;
	playerAction& state = player.paction;
	if (turnAround) TurnAround();
//	LogOut("NewState", TextFormat("%i", newState));



	if (!HasPixelAt(pos.x, pos.y) && newState == WALKING)
	{
		transitionState(FALLING);
	}
	if (newState == state) return;



	if (newState == ASCENDING)
	{
		player.Ascended = 0;
	}

	if (newState == FALLING)
	{

		player.Fallen = 1;
		if (state == WALKING)
		{

			player.Fallen = 3;
		}

		player.TrueFallen = player.Fallen;
	}

	state = newState;

}


void handleWalking()
{

	int DY = 0;
	Vector2& pos = player.position;
	Vector2& mov = player.movement;
	pos.x += mov.x;
	DY = findGroundPixel(pos.x, pos.y);

	if (DY < -6)
	{

		TurnAround();
		pos.x += mov.x;
	}
	else if (DY < -2)
	{

		transitionState(ASCENDING);
		pos.y += 2;


	}
	else if (DY < 1)
	{

		pos.y += DY;
	}
	DY = findGroundPixel(pos.x, pos.y);

	if (DY > 3)
	{
		pos.y += 4;
		transitionState(FALLING);

	}
	else if (DY > 0)
	{

		pos.y += DY;
	}

}
void handleFalling() {
	int curFallDisnace = 0;
	int maxFallDistance = 3;
	Vector2& pos = player.position;
	Vector2& mov = player.movement;
	while (curFallDisnace < maxFallDistance && !HasPixelAt(pos.x, pos.y))
	{
		pos.y++;
		curFallDisnace++;
		player.Fallen++;
		player.TrueFallen++;



	}

	if (player.Fallen > MAXFALLDISTANCE) player.Fallen = MAXFALLDISTANCE + 1;
	if (player.TrueFallen > MAXFALLDISTANCE) player.TrueFallen = MAXFALLDISTANCE + 1;


	if (curFallDisnace < maxFallDistance)
	{

		transitionState(WALKING);
		return;
	}
}
void handleAscending() {
	int DY = 0;
	Vector2& pos = player.position;
	Vector2& mov = player.movement;
	int& asc = player.Ascended;

	while (DY < 8 && asc < 8 && HasPixelAt(pos.x, pos.y - 1))
	{

		DY++;
		pos.y--;
		player.Ascended++;
	}

	if (DY < 2 && !HasPixelAt(pos.x, pos.y - 1))
	{
		//pos.y-=4;

		transitionState(WALKING);
		return;
	}
	else if (
		(asc == 4 && HasPixelAt(pos.x, pos.y - 1) && HasPixelAt(pos.x, pos.y - 2)) ||
		(asc >= 5 && HasPixelAt(pos.x, pos.y - 1)))
	{

		pos.x -= mov.x;
		transitionState(FALLING, true);

	}


}
void handleStanding() {}


void setupBGMask()
{
	maskBg = (int*)calloc(Size, sizeof(int));
	Color* cols = LoadImageColors(imgBg);
	for (int y = 0; y < Height; y++)
	{

		for (int x = 0; x < Width; x++)
		{
			int ix = y * Width + x;

			if (ix >= Size || ix <= 0) continue;

			if (cols[ix].a > 0)
			{

				maskBg[ix] = 1;
			}
			else {
				maskBg[ix] = 0;
			}


		}
	}
	UnloadImageColors(cols);
}
void setupBombMask()
{
	maskBomb = (int*)calloc(bombSize, sizeof(int));
	Color* cols = LoadImageColors(imgBomb);
	for (int y = 0; y < bombHeight; y++)
		for (int x = 0; x < bombWidth; x++)
		{
			int ix = y * bombWidth + x;
			if (ix >= bombSize || ix <= 0) continue;
			if (cols[ix].a != 0)
			{
				maskBomb[ix] = 1;
			}
			else
			{

				maskBomb[ix] = 0;
			}

		}

	UnloadImageColors(cols);
}

void cutBombMask(int cx, int cy)
{

	//there are better ways of doing this with ptr logic! see raylib's imagedraw for ideas,i've done this to illustrate what's happening.


//	Color* c = LoadImageColors(imgBg);

	cx = cx - bombWidth / 2;
	cy = cy - bombHeight;

	for (int y = 0; y < bombHeight; y++)
	{
		for (int x = 0; x < bombWidth; x++)
		{
			int sx = y * bombWidth + x;
			int dx = (cy + y) * Width + (cx + x);
			if (dx > Size || sx > bombSize || dx < 0 || sx < 0) continue;

			if (maskBomb[sx] == 1)
			{
				maskBg[dx] = 0;
			}



		}
	}




	//	UnloadImageColors(c);
	imgInvalid = true;

}

void cutPx(int _x, int _y)
{

	for (int y = _y - 5; y < _y + 5; y++)
		for (int x = _x - 5; x < _x + 5; x++)
		{
			int ix = y * Width + x;

			if (ix >= Size || ix <= 0) continue;
			maskBg[ix] = 0;

		}



}


int HasPixelAt(int x, int y)
{
	return GetImageColor(imgBg, x, y).a == 255;
}
int findGroundPixel(int x, int y)
{
	int r = 0;
	if (HasPixelAt(x, y))
	{
		while (HasPixelAt(x, y + r - 1) && (r > -7))
		{
			r--;
		}
	}
	else {
		r++;
		while (!HasPixelAt(x, y + r) && (r < 4))
		{

			r++;
		}
	}


	return r;
}





#ifdef OLD

//----------------------------------------------------------------------------------
// Shared Variables Definition (global)
// NOTE: Those variables are shared between modules through screens.h
//----------------------------------------------------------------------------------
GameScreen currentScreen = 0;
Font font = { 0 };
Music music = { 0 };
Sound fxCoin = { 0 };

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 800;
static const int screenHeight = 450;

// Required variables to manage screen transitions (fade-in, fade-out)
static float transAlpha = 0.0f;
static bool onTransition = false;
static bool transFadeOut = false;
static int transFromScreen = -1;
static int transToScreen = -1;

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void ChangeToScreen(int screen);     // Change to screen, no transition effect

static void TransitionToScreen(int screen); // Request transition to next screen
static void UpdateTransition(void);         // Update transition effect
static void DrawTransition(void);           // Draw transition effect (full-screen rectangle)

static void UpdateDrawFrame(void);          // Update and draw one frame

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "raylib game template");

    InitAudioDevice();      // Initialize audio device

    // Load global data (assets that must be available in all screens, i.e. font)
    font = LoadFont("resources/mecha.png");
    music = LoadMusicStream("resources/ambient.ogg");
    fxCoin = LoadSound("resources/coin.wav");

    SetMusicVolume(music, 1.0f);
    PlayMusicStream(music);

    // Setup and init first screen
    currentScreen = LOGO;
    InitLogoScreen();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload current screen data before closing
    switch (currentScreen)
    {
        case LOGO: UnloadLogoScreen(); break;
        case TITLE: UnloadTitleScreen(); break;
        case GAMEPLAY: UnloadGameplayScreen(); break;
        case ENDING: UnloadEndingScreen(); break;
        default: break;
    }

    // Unload global data loaded
    UnloadFont(font);
    UnloadMusicStream(music);
    UnloadSound(fxCoin);

    CloseAudioDevice();     // Close audio context

    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------
// Change to next screen, no transition
static void ChangeToScreen(int screen)
{
    // Unload current screen
    switch (currentScreen)
    {
        case LOGO: UnloadLogoScreen(); break;
        case TITLE: UnloadTitleScreen(); break;
        case GAMEPLAY: UnloadGameplayScreen(); break;
        case ENDING: UnloadEndingScreen(); break;
        default: break;
    }

    // Init next screen
    switch (screen)
    {
        case LOGO: InitLogoScreen(); break;
        case TITLE: InitTitleScreen(); break;
        case GAMEPLAY: InitGameplayScreen(); break;
        case ENDING: InitEndingScreen(); break;
        default: break;
    }

    currentScreen = screen;
}

// Request transition to next screen
static void TransitionToScreen(int screen)
{
    onTransition = true;
    transFadeOut = false;
    transFromScreen = currentScreen;
    transToScreen = screen;
    transAlpha = 0.0f;
}

// Update transition effect (fade-in, fade-out)
static void UpdateTransition(void)
{
    if (!transFadeOut)
    {
        transAlpha += 0.05f;

        // NOTE: Due to float internal representation, condition jumps on 1.0f instead of 1.05f
        // For that reason we compare against 1.01f, to avoid last frame loading stop
        if (transAlpha > 1.01f)
        {
            transAlpha = 1.0f;

            // Unload current screen
            switch (transFromScreen)
            {
                case LOGO: UnloadLogoScreen(); break;
                case TITLE: UnloadTitleScreen(); break;
                case OPTIONS: UnloadOptionsScreen(); break;
                case GAMEPLAY: UnloadGameplayScreen(); break;
                case ENDING: UnloadEndingScreen(); break;
                default: break;
            }

            // Load next screen
            switch (transToScreen)
            {
                case LOGO: InitLogoScreen(); break;
                case TITLE: InitTitleScreen(); break;
                case GAMEPLAY: InitGameplayScreen(); break;
                case ENDING: InitEndingScreen(); break;
                default: break;
            }

            currentScreen = transToScreen;

            // Activate fade out effect to next loaded screen
            transFadeOut = true;
        }
    }
    else  // Transition fade out logic
    {
        transAlpha -= 0.02f;

        if (transAlpha < -0.01f)
        {
            transAlpha = 0.0f;
            transFadeOut = false;
            onTransition = false;
            transFromScreen = -1;
            transToScreen = -1;
        }
    }
}

// Draw transition effect (full-screen rectangle)
static void DrawTransition(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, transAlpha));
}

// Update and draw game frame
static void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    UpdateMusicStream(music);       // NOTE: Music keeps playing between screens

    if (!onTransition)
    {
        switch(currentScreen)
        {
            case LOGO:
            {
                UpdateLogoScreen();

                if (FinishLogoScreen()) TransitionToScreen(TITLE);

            } break;
            case TITLE:
            {
                UpdateTitleScreen();

                if (FinishTitleScreen() == 1) TransitionToScreen(OPTIONS);
                else if (FinishTitleScreen() == 2) TransitionToScreen(GAMEPLAY);

            } break;
            case OPTIONS:
            {
                UpdateOptionsScreen();

                if (FinishOptionsScreen()) TransitionToScreen(TITLE);

            } break;
            case GAMEPLAY:
            {
                UpdateGameplayScreen();

                if (FinishGameplayScreen() == 1) TransitionToScreen(ENDING);
                //else if (FinishGameplayScreen() == 2) TransitionToScreen(TITLE);

            } break;
            case ENDING:
            {
                UpdateEndingScreen();

                if (FinishEndingScreen() == 1) TransitionToScreen(TITLE);

            } break;
            default: break;
        }
    }
    else UpdateTransition();    // Update transition (fade-in, fade-out)
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        switch(currentScreen)
        {
            case LOGO: DrawLogoScreen(); break;
            case TITLE: DrawTitleScreen(); break;
            case OPTIONS: DrawOptionsScreen(); break;
            case GAMEPLAY: DrawGameplayScreen(); break;
            case ENDING: DrawEndingScreen(); break;
            default: break;
        }

        // Draw full screen rectangle in front of everything
        if (onTransition) DrawTransition();

        //DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
}
#endif