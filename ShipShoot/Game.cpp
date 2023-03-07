#include "Game.h"
#include "D3D.h"
#include "WindowUtils.h"
#include "CommonStates.h"
//#include "Timer.h"		// for timing
#include "GeometryBuilder.h"

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
AudioMgrFMOD audio;

void Setup(Model& m, Mesh& source, const Vector3& scale, const Vector3& pos, const Vector3& rot)
{
	m.Initialise(source);
	m.GetScale() = scale;
	m.GetPosition() = pos;
	m.GetRotation() = rot;
}

void Setup(Model& m, Mesh& source, float scale, const Vector3& pos, const Vector3& rot)
{
	Setup(m, source, Vector3(scale, scale, scale), pos, rot);
}

void Game::Load()
{
	MyD3D& d3d = WinUtil::Get().GetD3D();

	Model m;
	mModels.insert(mModels.begin(), Modelid::TOTAL, m);

	Mesh& quadMesh = BuildQuad(d3d.GetMeshMgr());
	Mesh& cubeMesh = BuildCube(d3d.GetMeshMgr());

	//textured lit box
	mModels[Modelid::BOX].Initialise(cubeMesh);
	mModels[Modelid::BOX].GetPosition() = Vector3(0, -0.5f, 1);
	mModels[Modelid::BOX].GetScale() = Vector3(0.5f, 0.5f, 0.5f);
	Material mat = mModels[3].GetMesh().GetSubMesh(0).material;
	mat.gfxData.Set(Vector4(0.5, 0.5, 0.5, 1), Vector4(1, 1, 1, 0), Vector4(1, 1, 1, 1));
	mat.pTextureRV = d3d.GetCache().LoadTexture(&d3d.GetDevice(), "tiles.dds");
	mat.texture = "tiles.dds";
	mat.flags |= Material::TFlags::TRANSPARENCY;
	mat.SetBlendFactors(0.5, 0.5, 0.5, 1);
	mModels[Modelid::BOX].SetOverrideMat(&mat);

	//cross
	mModels[Modelid::CROSS].Initialise(cubeMesh);
	mat.flags &= ~Material::TFlags::TRANSPARENCY;
	mModels[Modelid::CROSS].GetScale() = Vector3(0.5f, 0.5f, 0.5f);
	mModels[Modelid::CROSS].GetPosition() = Vector3(1.5f, -0.45f, 1);
	mat.pTextureRV = d3d.GetCache().LoadTexture(&d3d.GetDevice(), "cross.dds");
	mat.texture = "cross";
	mat.flags |= Material::TFlags::ALPHA_TRANSPARENCY;
	mat.flags &= ~Material::TFlags::CCW_WINDING;	//render the back
	mModels[Modelid::CROSS].SetOverrideMat(&mat);

	mModels[Modelid::CROSS2] = mModels[Modelid::CROSS];
	mat.flags |= Material::TFlags::CCW_WINDING;	//render the front
	mModels[Modelid::CROSS2].SetOverrideMat(&mat);

	//window
	mModels[Modelid::WINDOW].Initialise(cubeMesh);
	mModels[Modelid::WINDOW].GetScale() = Vector3(0.75f, 0.75f, 0.75f);
	mModels[Modelid::WINDOW].GetPosition() = Vector3(-1.75f, 0, 1.25f);
	mat.pTextureRV = d3d.GetCache().LoadTexture(&d3d.GetDevice(), "alphaWindow.dds");
	mat.texture = "alphawindow";
	mat.flags |= Material::TFlags::ALPHA_TRANSPARENCY;
	mat.flags &= ~Material::TFlags::CCW_WINDING;	//render the back
	mModels[Modelid::WINDOW].SetOverrideMat(&mat);

	mModels[Modelid::WINDOW2] = mModels[Modelid::WINDOW];
	mat.flags |= Material::TFlags::CCW_WINDING;	//render the front
	mModels[Modelid::WINDOW2].SetOverrideMat(&mat);

	//quad wood floor
	Setup(mModels[Modelid::FLOOR], quadMesh, Vector3(3, 1, 3), Vector3(0, -1, 0), Vector3(0, 0, 0));
	mat = mModels[Modelid::FLOOR].GetMesh().GetSubMesh(0).material;
	mat.gfxData.Set(Vector4(0.9f, 0.8f, 0.8f, 0), Vector4(0.9f, 0.8f, 0.8f, 0), Vector4(0.9f, 0.8f, 0.8f, 1));
	mat.pTextureRV = d3d.GetCache().LoadTexture(&d3d.GetDevice(), "floor.dds");
	mat.texture = "floor.dds";
	mModels[Modelid::FLOOR].SetOverrideMat(&mat);

	//back wall
	Setup(mModels[Modelid::BACK_WALL], quadMesh, Vector3(3, 1, 1.5f), Vector3(0, 0.5f, 3), Vector3(-PI / 2, 0, 0));
	mat.gfxData.Set(Vector4(1, 1, 1, 0), Vector4(1, 1, 1, 0), Vector4(1, 1, 1, 1));
	mat.pTextureRV = d3d.GetCache().LoadTexture(&d3d.GetDevice(), "wall.dds");
	mat.texture = "wall.dds";
	mModels[Modelid::BACK_WALL].SetOverrideMat(&mat);

	//left wall
	Setup(mModels[Modelid::LEFT_WALL], quadMesh, Vector3(3, 1, 1.5f), Vector3(-3, 0.5f, 0), Vector3(-PI / 2, -PI / 2, 0));
	mModels[Modelid::LEFT_WALL].SetOverrideMat(&mat);
	mLoadData.loadedSoFar++;

	//hero models
	Mesh& cb = d3d.GetMeshMgr().CreateMesh("Cube");
	cb.CreateFrom("../bin/data/two_mat_cube.fbx", d3d);
	Setup(mModels[Modelid::SUCK], cb, 0.045f, Vector3(1, 0.25f, -1.5f), Vector3(PI / 2.f, 0, 0));
	mLoadData.loadedSoFar++;

	Mesh& ms = d3d.GetMeshMgr().CreateMesh("rock");
	ms.CreateFrom("../bin/data/the_rock/TheRock2.obj", d3d);
	Setup(mModels[Modelid::ROCK], ms, 0.0035f, Vector3(-1, 0.25f, -2.5f), Vector3(0, 0, 0));
	mLoadData.loadedSoFar++;

	Mesh& dr = d3d.GetMeshMgr().CreateMesh("dragon");
	dr.CreateFrom("../bin/data/dragon/dragon.x", d3d);
	Setup(mModels[Modelid::DRAGON], dr, 0.002f, Vector3(-1, 0.f, -1.5f), Vector3(0, PI, 0));
	mLoadData.loadedSoFar++;

	Mesh& dx = d3d.GetMeshMgr().CreateMesh("DrX");
	dx.CreateFrom("../bin/data/drX/DrX.fbx", d3d);
	Setup(mModels[Modelid::SCIENTIST], dx, .075f, Vector3(1, 0.5f, -2.5f), Vector3(PI / 2, PI, 0));
	mLoadData.loadedSoFar++;

	Mesh& sm = d3d.GetMeshMgr().CreateMesh("Mole");
	sm.CreateFrom("../bin/data/mole/mole.fbx", d3d);
	Setup(mModels[Modelid::MOLE], sm, 0.045f, Vector3(0, 0.f, -2.f), Vector3(PI / 2, 0, 0));
	mLoadData.loadedSoFar++;

	d3d.GetFX().SetupDirectionalLight(0, true, Vector3(-0.7f, -0.7f, 0.7f), Vector3(0.47f, 0.47f, 0.47f), Vector3(0.15f, 0.15f, 0.15f), Vector3(0.25f, 0.25f, 0.25f));
}

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

void Game::RenderLoad(float dTime)
{
	MyD3D& d3d = WinUtil::Get().GetD3D();
	d3d.BeginRender(Colours::Black);

	pFontBatch->Begin();

	static int pips = 0;
	static float elapsed = 0;
	elapsed += dTime;
	if (elapsed > 0.25f) {
		pips++;
		elapsed = 0;
	}
	if (pips > 10)
		pips = 0;
	wstringstream ss;
	ss << L"Loading meshes(" << (int)(((float)mLoadData.loadedSoFar / (float)mLoadData.totalToLoad) * 100.f) << L"%) ";
	for (int i = 0; i < pips; ++i)
		ss << L'.';
	pFont->DrawString(pFontBatch, ss.str().c_str(), Vector2(100, 200), Colours::White, 0, Vector2(0, 0), Vector2(1.f, 1.f));


	ss.str(L"");
	ss << L"FPS:" << (int)(1.f / dTime);
	pFont->DrawString(pFontBatch, ss.str().c_str(), Vector2(10, 550), Colours::White, 0, Vector2(0, 0), Vector2(0.5f, 0.5f));


	pFontBatch->End();


	d3d.EndRender();

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
			mPMode.InitSound(dTime);
			audio.Update();
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

void Game::Render3D(float dTime)
{
	if (mLoadData.running)
	{
		if (!mLoadData.loader._Is_ready())
		{
			RenderLoad(dTime);
			return;
		}
		mLoadData.loader.get();
		mLoadData.running = false;
		return;
	}

	MyD3D& d3d = WinUtil::Get().GetD3D();
	d3d.BeginRender(Colours::Black);

	float alpha = 0.5f + sinf(gAngle * 2) * 0.5f;

	d3d.GetFX().SetPerFrameConsts(d3d.GetDeviceCtx(), mCamPos);

	CreateViewMatrix(d3d.GetFX().GetViewMatrix(), mCamPos, Vector3(0, 0, 0), Vector3(0, 1, 0));
	CreateProjectionMatrix(d3d.GetFX().GetProjectionMatrix(), 0.25f * PI, WinUtil::Get().GetAspectRatio(), 1, 1000.f);
	Matrix w = Matrix::CreateRotationY(sinf(gAngle));
	d3d.GetFX().SetPerObjConsts(d3d.GetDeviceCtx(), w);

	//main cube - forced transparency under pogram control
	Vector3 dir = Vector3(1, 0, 0);
	Matrix m = Matrix::CreateRotationY(gAngle);
	dir = dir.TransformNormal(dir, m);
	d3d.GetFX().SetupSpotLight(1, true, mModels[Modelid::BOX].GetPosition(), dir, Vector3(0.2f, 0.05f, 0.05f), Vector3(0.01f, 0.01f, 0.01f), Vector3(0.01f, 0.01f, 0.01f));
	dir *= -1;
	d3d.GetFX().SetupSpotLight(2, true, mModels[Modelid::BOX].GetPosition(), dir, Vector3(0.05f, 0.2f, 0.05f), Vector3(0.01f, 0.01f, 0.01f), Vector3(0.01f, 0.01f, 0.01f));
	float d = sinf(gAngle) * 0.5f + 0.5f;
	mModels[Modelid::BOX].HasOverrideMat()->SetBlendFactors(d, d, d, 1);



	for (auto& mod : mModels)
		d3d.GetFX().Render(mod);

	d3d.EndRender();
}

void Game::Initialise()
{
	MyD3D& d3d = WinUtil::Get().GetD3D();
	pFontBatch = new SpriteBatch(&d3d.GetDeviceCtx());
	assert(pFontBatch);
	pFont = new SpriteFont(&d3d.GetDevice(), L"../bin/data/fonts/algerian.spritefont");
	assert(pFont);

	mLoadData.totalToLoad = 5;
	mLoadData.loadedSoFar = 0;
	mLoadData.running = true;
	mLoadData.loader = std::async(launch::async, &Game::Load, this);
}

PlayMode::PlayMode(MyD3D& d3d)
	:mD3D(d3d), mPlayer(d3d), mMoleBgnd(d3d), mMole(d3d), mEndScreen(d3d)
{
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

	if (((sprite1.GetScreenSize().x)/2 + (sprite2.GetScreenSize().y)/2) > origin_dis_x && 
		((sprite1.GetScreenSize().y) / 2 + (sprite2.GetScreenSize().x) / 2) > origin_dis_y)
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
	{ {455, 220}, {845, 235}, {1220, 235},
	{455, 495}, {845, 495}, {1220, 495},
	{455, 795}, {845, 795}, {1220, 795} };
	
	srand(time(0));
	int random_hole_pos = rand() % hole_coordinates.size();

	sprite1.mPos = Vector2{ (hole_coordinates[random_hole_pos].x)/2, hole_coordinates[random_hole_pos].y/2};
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

void PlayMode::Update(float dTime)
{
	UpdateInput(dTime);
}

void Game::Update3D(float dTime)
{
	gAngle += dTime * 0.5f;
	mModels[Modelid::BOX].GetRotation().y = gAngle;

	mModels[Modelid::CROSS].GetRotation().y = -gAngle;
	mModels[Modelid::CROSS2].GetRotation().y = -gAngle;

	mModels[Modelid::WINDOW].GetRotation().y = -gAngle * 0.5f;
	mModels[Modelid::WINDOW2].GetRotation().y = -gAngle * 0.5f;

	vector<Model*> spinme{ &mModels[Modelid::DRAGON], &mModels[Modelid::ROCK], &mModels[Modelid::SCIENTIST], &mModels[Modelid::SUCK], &mModels[Modelid::MOLE] };

	for (size_t i = 0; i < spinme.size(); ++i) {
		spinme[i]->GetPosition().y = (sinf(2 * GetClock() + (PI / 4) * i)) * 0.5f;
		spinme[i]->GetRotation().y += (i < 2) ? dTime : -dTime;
	}
}

void PlayMode::UpdateEnd(float dTime)
{
	game_time = 20;
	score = 0;

	audio.Update();
	audio.GetSfxMgr()->Play("click", false, false, &sfxHdl, 0.02f);
}

void PlayMode::UpdateEndSound(float dTime)
{
	audio.Update();
	audio.GetSfxMgr()->Play("gameover", false, false, &sfxHdl, 0.1f);
}

void PlayMode::UpdateIntro(float dTime)
{
	//audio.Initialise();
	if (!(audio.GetSongMgr()->IsPlaying(musicHdl)))
		audio.GetSongMgr()->Play("kirby_short", true, false, &musicHdl, 0.2f);
	audio.GetSfxMgr()->Play("click_cut", false, false, &sfxHdl, 0.1f);
	audio.Update();	
	//anything that needs to be changed in the into
}

void PlayMode::InitSound(float dTime)
{
	audio.Initialise();
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

		audio.Update();
		audio.GetSfxMgr()->Play("spring", true, false, &sfxHdl, 0.1f);
		set_random_pos(mMole);
		score += 1;
		//DBOUT(score);
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
	mMole.origin = mMole.GetTexData().dim / 2;
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
}

void PlayMode::InitEndScreen()
{
	ID3D11ShaderResourceView* p = mD3D.GetCache().LoadTexture(&mD3D.GetDevice(), "grey_screen.dds");
	mEndScreen.SetTex(*p);
	mEndScreen.SetScale(Vector2(2.f, 2.f));
	mEndScreen.origin = mPlayer.GetTexData().dim / 2.f;
}