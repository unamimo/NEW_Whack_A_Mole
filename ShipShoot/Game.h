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
	void Render(float dTime, DirectX::SpriteBatch& batch, DirectX::SpriteFont& font);
	bool check_collisions(Sprite& sprite1, Sprite& sprite2);
	void set_random_pos(Sprite& sprite1);
private:
	const float SCROLL_SPEED = 10.f;
	static const int BGND_LAYERS = 8;
	const float SPEED = 250;
	const float MOUSE_SPEED = 5000;
	const float PAD_SPEED = 500;

	MyD3D& mD3D;
	std::vector<Sprite> mBgnd; //parallax layers
	Sprite mMoleBgnd;   //mole bgnd
	std::vector<Sprite> mMole;		//multiple moles
	Sprite mPlayer;		//jet
	//Sprite MoleSpr;
	RECTF mPlayArea;	//don't go outside this	
	Sprite mThrust;		//flames out the back
	Bullet mMissile;	//weapon, only one at once
	
	//once we start thrusting we have to keep doing it for 
	//at least a fraction of a second or it looks whack
	float mThrusting = 0; 

	//setup once
	void InitBgnd();
	void InitMoleBgnd();
	void InitPlayer();
	void InitMole();

	//make it move, reset it once it leaves the screen, only one at once
	void UpdateMissile(float dTime);
	//make it scroll parallax
	void UpdateBgnd(float dTime);
	//move the ship by keyboard, gamepad or mouse
	void UpdateInput(float dTime);
	//make the flames wobble when the ship moves
	void UpdateThrust(float dTime);
};


/*
Basic wrapper for a game
*/
class Game
{
public:
	enum class State { PLAY };
	static MouseAndKeys sMKIn;
	static Gamepads sGamepads;
	State state = State::PLAY;
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
	const int num_moles = 9;
};


