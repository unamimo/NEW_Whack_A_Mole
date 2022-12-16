#pragma once

#include <vector>

#include "Input.h"
#include "D3D.h"
#include "SpriteBatch.h"
#include "Sprite.h"
#include "SpriteFont.h"

/*
Animated missile bullet 
Player can only fire one and has to wait for it to leave the 
screen before firing again.
*/
struct Bullet
{
	Bullet(MyD3D& d3d)
		:bullet(d3d)
	{}
	Sprite bullet;
	bool active = false;

	void Init(MyD3D& d3d);
	void Render(DirectX::SpriteBatch& batch);
	void Update(float dTime);
	const float MISSILE_SPEED = 300;
};


//horizontal scrolling with player controlled ship
class PlayMode
{
public:
	PlayMode(MyD3D& d3d);
	void Update(float dTime);
	void UpdateEnd(float dTime);
	void Render(float dTime, DirectX::SpriteBatch& batch, DirectX::SpriteFont* font);
	void RenderEnd(float dTime, DirectX::SpriteBatch& batch, DirectX::SpriteFont* font);
	bool check_collisions(Sprite& sprite1, Sprite& sprite2);
	void set_random_pos(Sprite& sprite1);
	bool check_time(float dTime);
private:
	const float SCROLL_SPEED = 10.f;
	static const int BGND_LAYERS = 8;
	const float SPEED = 250;
	const float MOUSE_SPEED = 5000;
	const float PAD_SPEED = 500;
	bool mouseAlreadyDown = false;
	bool keyAlreadyDown = false;
	float game_time = 10;
	int score = 0;

	MyD3D& mD3D;
	std::vector<Sprite> mBgnd; //parallax layers
	Sprite mMoleBgnd;   //mole bgnd
	Sprite mMole;		//moles
	Sprite mPlayer;		//hammer
	Sprite mEndScreen;
	RECTF mPlayArea;
	Sprite mThrust;	
	Bullet mMissile;
	
	//once we start thrusting we have to keep doing it for 
	//at least a fraction of a second or it looks whack
	float mThrusting = 0; 

	//setup once
	void InitBgnd();
	void InitMoleBgnd();
	void InitPlayer();
	void InitMole();
	void InitEndScreen();

	//make it move, reset it once it leaves the screen, only one at once
	void UpdateMissile(float dTime);
	//make it scroll parallax
	void UpdateBgnd(float dTime);
	//move the ship by keyboard, gamepad or mouse
	void UpdateInput(float dTime);
	//make the flames wobble when the ship moves
	void UpdateThrust(float dTime);
	//time the game
	void UpdateTimer(float dTime);
};


/*
Basic wrapper for a game
*/
class Game
{
public:
	enum class State { PLAY, END } game_state = State::PLAY;
	static MouseAndKeys sMKIn;
	static Gamepads sGamepads;
	Game(MyD3D& d3d);


	void Release();
	void Update(float dTime);
	void Render(float dTime);
private:
	MyD3D& mD3D;
	DirectX::SpriteBatch *mpSB = nullptr;
	DirectX::SpriteFont *pFont = nullptr;
	//not much of a game, but this is it
	PlayMode mPMode;
};


