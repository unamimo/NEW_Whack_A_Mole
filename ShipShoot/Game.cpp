#include "Game.h"
#include "WindowUtils.h"
#include "CommonStates.h"
//#include "Timer.h"		// for timing
#include <iostream>
#include <thread>
#include <cstdlib>		// for srand()
#include <ctime>		// in use with srand() 
#include <cmath>		//trunc() - removes decimals
#include <fstream>		// for high score storing


using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, seconds

MouseAndKeys Game::sMKIn;
Gamepads Game::sGamepads;

const RECTF missileSpin[]{
	{ 0,  0, 53, 48},
	{ 54, 0, 107, 48 },
	{ 108, 0, 161, 48 },
	{ 162, 0, 220, 48 },
};

const RECTF thrustAnim[]{
	{ 0,  0, 15, 16},
	{ 16, 0, 31, 16 },
	{ 32, 0, 47, 16 },
	{ 48, 0, 64, 16 },
};

Game::Game(MyD3D& d3d)
	: mPMode(d3d), mD3D(d3d), mpSB(nullptr), pFont(nullptr)
{
	sMKIn.Initialise(WinUtil::Get().GetMainWnd(), true, false);
	sGamepads.Initialise();
	mpSB = new SpriteBatch(&mD3D.GetDeviceCtx());

	//for text
	pFont = new SpriteFont(&mD3D.GetDevice(), L"data/fonts/comicSansMS.spritefont");
}


//any memory or resources we made need releasing at the end
void Game::Release()
{
	delete mpSB;
	delete pFont;
	mpSB = nullptr;
	pFont = nullptr;
}

//called over and over, use it to update game logic
void Game::Update(float dTime)
{
	sGamepads.Update();
	switch (game_state)
	{
	case State::INTRO:
		if (Game::sMKIn.IsPressed(VK_P))
		{
			mPMode.UpdateIntro(dTime);
			game_state = State::PLAY;
		}
	case State::PLAY:
		mPMode.Update(dTime);
		if (mPMode.check_time(dTime) == true)
		{
			game_state = State::END; //End the game
			break;
		}
	case State::END:
		if (Game::sMKIn.IsPressed(VK_R))
		{
			mPMode.UpdateEnd(dTime);
			game_state = State::PLAY;
		}
	}
}

//called over and over, use it to render things
void Game::Render(float dTime)
{
	mD3D.BeginRender(Colours::Black);


	CommonStates dxstate(&mD3D.GetDevice());
	mpSB->Begin(SpriteSortMode_Deferred, dxstate.NonPremultiplied(), &mD3D.GetWrapSampler());

	switch (game_state)
	{
	case State::PLAY:
		mPMode.Render(dTime, *mpSB, pFont);
		if (mPMode.check_time(dTime) == true)
		{
			game_state = State::END; //End the game
			break;
		}
		break;

	case State::END:
		mPMode.RenderEnd(dTime, *mpSB, pFont);
		break;

	case State::INTRO:
		mPMode.RenderIntro(dTime, *mpSB, pFont);
		break;
	}

	mpSB->End();


	mD3D.EndRender();
	sMKIn.PostProcess();
}

void Bullet::Init(MyD3D& d3d)
{
	vector<RECTF> frames2(missileSpin, missileSpin + sizeof(missileSpin) / sizeof(missileSpin[0]));
	ID3D11ShaderResourceView* p = d3d.GetCache().LoadTexture(&d3d.GetDevice(), "missile.dds", "missile", true, &frames2);

	bullet.SetTex(*p);
	bullet.GetAnim().Init(0, 3, 15, true);
	bullet.GetAnim().Play(true);
	bullet.SetScale(Vector2(0.5f, 0.5f));
	bullet.origin = Vector2((missileSpin[0].right - missileSpin[0].left) / 2.f, (missileSpin[0].bottom - missileSpin[0].top) / 2.f);
	active = false;
}

void Bullet::Render(SpriteBatch& batch)
{
	if (active)
		bullet.Draw(batch);
}

void Bullet::Update(float dTime)
{
	if (active)
	{
		bullet.mPos.x += MISSILE_SPEED * dTime;
		if (bullet.mPos.x > WinUtil::Get().GetClientWidth())
			active = false;
		bullet.GetAnim().Update(dTime);
	}
}

PlayMode::PlayMode(MyD3D& d3d)
	:mD3D(d3d), mPlayer(d3d), mThrust(d3d), mMissile(d3d), mMoleBgnd(d3d), mMole(d3d), mEndScreen(d3d)
{
	InitBgnd();
	InitMoleBgnd();
	InitPlayer();
	InitMole();
	InitEndScreen();
}

bool PlayMode::check_collisions(Sprite& sprite1, Sprite& sprite2)
{
	//distance between origins
	float origin_dis_x = fabs(sprite1.mPos.x - sprite2.mPos.x);
	float origin_dis_y = fabs(sprite1.mPos.y - sprite2.mPos.y);
	
	if ((sprite1.colour == Vector4(0, 255, 0, 0)) || (sprite2.colour == Vector4(0, 255, 0, 0)))
	{
		return false;
	}

	if (((sprite1.GetScreenSize().x)/2 + (sprite2.GetScreenSize().x)/2) > origin_dis_x && 
		((sprite1.GetScreenSize().y) / 2 + (sprite2.GetScreenSize().y) / 2) > origin_dis_y)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void PlayMode::set_random_pos(Sprite& sprite1)
{
	vector<Vector2> hole_coordinates =
	{ {142, 20}, {335, 20}, {520, 20},
	{142, 165}, {335, 165}, {530, 165},
	{142, 310}, {335, 310}, {530, 310} };
	
	srand(time(0));
	int random_hole_pos = rand() % hole_coordinates.size();

	sprite1.mPos = Vector2{ hole_coordinates[random_hole_pos].x, hole_coordinates[random_hole_pos].y };
}

void PlayMode::high_scores(DirectX::SpriteBatch& batch, DirectX::SpriteFont* font)
{	
	std::ifstream inFile;	//input file
	std::ofstream outFile;	//output file

	inFile.open("best_scores.txt");
	if (!inFile.is_open())
	{
		DBOUT("Unable to read input file");
	}

	int best_score = score;
	inFile >> best_score;

	stringstream high_score_text;
	high_score_text << "High Score: " << best_score;

	outFile.open("best_scores.txt");
	if (!outFile.is_open())
	{
		DBOUT("Unable to read output file");
	}

	if (best_score < score)
	{
		outFile << score;
		DBOUT(score);
		font->DrawString(&batch, high_score_text.str().c_str(), Vector2(336, 200), Colours::Black, 0.f, Vector2(0, 0), Vector2(1, 1));
	}
	else
	{
		outFile << best_score;
		DBOUT(best_score);
		font->DrawString(&batch, high_score_text.str().c_str(), Vector2(336, 250), Colours::Black, 0.f, Vector2(0, 0), Vector2(1, 1));
	}

	inFile.close();
	outFile.close();
	return;
}

void PlayMode::UpdateTimer(float dTime)
{
	game_time -= dTime;
	if (game_time <= 0)
	{
		game_time = 0;
	}
}

bool PlayMode::check_time(float dTime)
{
	bool game_time_end = false;
	if (game_time <= 0)
	{
		bool game_time_end = true;
		return true;
	}
}

void PlayMode::UpdateMissile(float dTime)
{
	if (!mMissile.active && Game::sMKIn.IsPressed(VK_SPACE))
	{
		mMissile.active = true;
		mMissile.bullet.mPos = Vector2(mPlayer.mPos.x + mPlayer.GetScreenSize().x / 2.f, mPlayer.mPos.y);
	}
	mMissile.Update(dTime);
}


void PlayMode::UpdateBgnd(float dTime)
{
	//scroll the background layers
	int i = 0;
	for (auto& s : mBgnd)
		s.Scroll(dTime * (i++) * SCROLL_SPEED, 0);
}

void PlayMode::UpdateInput(float dTime)
{
	Vector2 mouse{ Game::sMKIn.GetMousePos(false) };
	bool keypressed = Game::sMKIn.IsPressed(VK_UP) || Game::sMKIn.IsPressed(VK_DOWN) ||
		Game::sMKIn.IsPressed(VK_RIGHT) || Game::sMKIn.IsPressed(VK_LEFT);
	bool sticked = false;
	if (Game::sGamepads.IsConnected(0) &&
		(Game::sGamepads.GetState(0).leftStickX != 0))
		sticked = true;

	if (keypressed || (mouse.Length() > VERY_SMALL) || sticked)
	{
		//move the ship around
		Vector2 pos(0, 0);
		if (Game::sMKIn.IsPressed(VK_UP))
			pos.y -= SPEED * dTime;
		else if (Game::sMKIn.IsPressed(VK_DOWN))
			pos.y += SPEED * dTime;
		if (Game::sMKIn.IsPressed(VK_RIGHT))
			pos.x += SPEED * dTime;
		else if (Game::sMKIn.IsPressed(VK_LEFT))
			pos.x -= SPEED * dTime;

		pos += mouse;

		if (sticked)
		{
			DBOUT("left stick x=" << Game::sGamepads.GetState(0).leftStickX << " y=" << Game::sGamepads.GetState(0).leftStickY);
			pos.x += Game::sGamepads.GetState(0).leftStickX * PAD_SPEED * dTime;
			pos.y -= Game::sGamepads.GetState(0).leftStickY * PAD_SPEED * dTime;
		}

		//keep it within the play area
		pos += mPlayer.mPos;
		/*if (pos.x < mPlayArea.left)
			pos.x = mPlayArea.left;
		else if (pos.x > mPlayArea.right)
			pos.x = mPlayArea.right;*/
		/*if (pos.y < mPlayArea.top)
			pos.y = mPlayArea.top;
		else if (pos.y > mPlayArea.bottom)
			pos.y = mPlayArea.bottom;*/

		mPlayer.mPos = pos;
		mThrusting = GetClock() + 0.2f;
	}

	if ((Game::sMKIn.IsPressed(VK_SPACE)) || (Game::sMKIn.GetMouseButton(MouseAndKeys::ButtonT::LBUTTON)))
	{
		mPlayer.rotation = PI / 2.f;
	}
	else
	{
		mPlayer.rotation = 0;
	}
}

void PlayMode::UpdateThrust(float dTime)
{
	if (mThrusting)
	{
		mThrust.mPos = mPlayer.mPos;
		mThrust.mPos.x -= 25;
		mThrust.mPos.y -= 12;
		mThrust.SetScale(Vector2(1.5f, 1.5f));
		mThrust.GetAnim().Update(dTime);
	}
}

void PlayMode::Update(float dTime)
{
	UpdateBgnd(dTime);

	UpdateMissile(dTime);

	UpdateInput(dTime);

	UpdateThrust(dTime);
}

void PlayMode::UpdateEnd(float dTime)
{
	game_time = 20;
	score = 0;
}

void PlayMode::UpdateIntro(float dTime)
{
	///things here 
	//anything that needs to be changed in the into
}

void PlayMode::Render(float dTime, DirectX::SpriteBatch& batch, DirectX::SpriteFont* font)
{
	
	bool whacked = false; 
	stringstream ss;
	ss << "Score: " << score;
	stringstream timer_text;
	timer_text << "Time: " << game_time;

	mMoleBgnd.Draw(batch);

	mMole.Draw(batch);
	if (check_collisions(mMole, mPlayer) && (Game::sMKIn.IsPressed(VK_SPACE) && !keyAlreadyDown
		|| (Game::sMKIn.GetMouseButton(MouseAndKeys::ButtonT::LBUTTON) && !mouseAlreadyDown)))
	{
		mouseAlreadyDown = true;
		keyAlreadyDown = true;
		mMole.colour = Vector4(0, 255, 0, 0);
		if (mMole.colour == Vector4(0, 255, 0, 0))
		{
			whacked = true;
		}
		if (whacked)
		{
			mMole.colour = Vector4(1, 1, 1, 1);
			set_random_pos(mMole);
			score += 1;
			//DBOUT(score);
		}
	}
	if (!Game::sMKIn.GetMouseButton(MouseAndKeys::ButtonT::LBUTTON))
		mouseAlreadyDown = false;
	if (!Game::sMKIn.IsPressed(VK_SPACE))
		keyAlreadyDown = false;

	mPlayer.Draw(batch);
	font->DrawString(&batch, ss.str().c_str(), Vector2(0, 0), Colours::Black, 0.f, Vector2(0, 0), Vector2(1, 1));
	font->DrawString(&batch, timer_text.str().c_str(), Vector2(725, 0), Colours::Black, 0.f, Vector2(0, 0), Vector2(1, 1));

	Vector2 mouse{ Game::sMKIn.GetMousePos(true) };
	mPlayer.mPos = mouse;
	UpdateTimer(dTime);
	//DBOUT(game_time);
}

void PlayMode::RenderEnd(float dTime, DirectX::SpriteBatch& batch, DirectX::SpriteFont* font)
{
	mEndScreen.Draw(batch);

	high_scores(batch, font);

	string end_text = "Game Over";
	stringstream ss;
	ss << "Score: " << score;
	string replay_text = "Press R to replay";
	font->DrawString(&batch, end_text.c_str(), Vector2(200, 0), Colours::Red, 0.f, Vector2(0, 0), Vector2(4, 4));
	font->DrawString(&batch, ss.str().c_str(), Vector2(370, 200), Colours::Black, 0.f, Vector2(0, 0), Vector2(1, 1));
	font->DrawString(&batch, replay_text.c_str(), Vector2(336, 300), Colours::Black, 0.f, Vector2(0, 0), Vector2(1, 1));
}

void PlayMode::RenderIntro(float dTime, DirectX::SpriteBatch& batch, DirectX::SpriteFont* font)
{
	mEndScreen.Draw(batch);

	string end_text = "Whack a' Mole";
	stringstream ss;
	ss << "Score: " << score;
	string play_text = "Press P to play";
	font->DrawString(&batch, end_text.c_str(), Vector2(136, 0), Colours::Green, 0.f, Vector2(0, 0), Vector2(4, 4));
	font->DrawString(&batch, play_text.c_str(), Vector2(336, 300), Colours::Black, 0.f, Vector2(0, 0), Vector2(1, 1));
}

void PlayMode::InitBgnd()
{
	//a sprite for each layer
	assert(mBgnd.empty());
	mBgnd.insert(mBgnd.begin(), BGND_LAYERS, Sprite(mD3D));

	//a neat way to package pairs of things (nicknames and filenames)
	pair<string, string> files[BGND_LAYERS]{
		{ "bgnd0","backgroundlayers/mountains01_007.dds" },
		{ "bgnd1","backgroundlayers/mountains01_005.dds" },
		{ "bgnd2","backgroundlayers/mountains01_004.dds" },
		{ "bgnd3","backgroundlayers/mountains01_003.dds" },
		{ "bgnd4","backgroundlayers/mountains01_002.dds" },
		{ "bgnd5","backgroundlayers/mountains01_001.dds" },
		{ "bgnd6","backgroundlayers/mountains01_000.dds" },
		{ "bgnd7","backgroundlayers/mountains01_006.dds" }
	};
	int i = 0;
	for (auto& f : files)
	{
		//set each texture layer
		ID3D11ShaderResourceView* p = mD3D.GetCache().LoadTexture(&mD3D.GetDevice(), f.second, f.first);
		if (!p)
			assert(false);
		mBgnd[i].SetTex(*p);
		mBgnd[i].SetScale(Vector2(2, 2));
		i++;
	}

}

void PlayMode::InitMoleBgnd()
{	
	ID3D11ShaderResourceView* p = mD3D.GetCache().LoadTexture(&mD3D.GetDevice(), "background.dds");
	mMoleBgnd.SetTex(*p);
	mMoleBgnd.origin = mMoleBgnd.GetTexData().dim/20.f;
	mMoleBgnd.SetScale(Vector2(0.5, 0.5));
}

void PlayMode::InitMole()
{
	ID3D11ShaderResourceView* p = mD3D.GetCache().LoadTexture(&mD3D.GetDevice(), "mole.dds");
	mMole.SetTex(*p); 
	mMole.SetScale(Vector2(0.6f, 0.6f));
	set_random_pos(mMole);
}

void PlayMode::InitPlayer()
{
	//load and orientate the hammer
	ID3D11ShaderResourceView* p = mD3D.GetCache().LoadTexture(&mD3D.GetDevice(), "hammer.dds");
	mPlayer.SetTex(*p);
	mPlayer.SetScale(Vector2(0.3f, 0.3f));
	mPlayer.origin = mPlayer.GetTexData().dim/2;

	//setup the play area
	int w, h;
	Vector2 mouse{ Game::sMKIn.GetMousePos(false) };
	WinUtil::Get().GetClientExtents(w, h);
	mPlayArea.left = mPlayer.GetScreenSize().x * 0.6f;
	mPlayArea.top = mPlayer.GetScreenSize().y * 0.6f;
	mPlayArea.right = w - mPlayArea.left;
	mPlayArea.bottom = h * 0.75f;
	mPlayer.mPos = {0,0};

	vector<RECTF> frames(thrustAnim, thrustAnim + sizeof(thrustAnim) / sizeof(thrustAnim[0]));
	p = mD3D.GetCache().LoadTexture(&mD3D.GetDevice(), "thrust.dds", "thrust", true, &frames);
	mThrust.SetTex(*p);
	mThrust.GetAnim().Init(0, 3, 15, true);
	mThrust.GetAnim().Play(true);
	mThrust.rotation = PI / 2.f;

	mMissile.Init(mD3D);
}

void PlayMode::InitEndScreen()
{
	ID3D11ShaderResourceView* p = mD3D.GetCache().LoadTexture(&mD3D.GetDevice(), "grey_screen.dds");
	mEndScreen.SetTex(*p);
	mEndScreen.SetScale(Vector2(2.f, 2.f));
	mEndScreen.origin = mPlayer.GetTexData().dim / 2.f;
}