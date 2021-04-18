#include<iostream>
#include<algorithm>
#include<fstream>
#include<cstdlib>
#include<sstream>
#include<SDL.h>
#include<SDL_image.h>
#include<SDL_ttf.h>
#include<time.h>
#include<deque>

using namespace std;

const int WIDTH = 1200, HEIGHT = 600;
const int FPS = 60;

int OBSTACLENUM, ENEMYNUM, TOTAL;
int level;
int x_axis[50], y_axis[] = {80, 160, 320, 480};
bool running = true, play= false;
int option = 0;
int heli_frame = 0;
int bg_frame = 0;
int score_counter;
long long score;
float obstacle_speed;
string score_string;
//stringstream ss;
int create_missile = 0;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Surface *image = NULL;

//Heli
float heli_x = WIDTH / 6.0f, heli_y = HEIGHT / 2.0f, speed = 0.0f;
int heli_h, heli_w;

//Destination rect for the heli
SDL_Rect heli_rect;

SDL_Texture *heli[5];
SDL_Texture *bomb[24];
SDL_Texture *enemy[5];
SDL_Texture *background[80];

//Score structure
typedef struct{
	int score;
	string name;
}score_struct;

//Compare function for sorting score structure
bool sort_score(score_struct  a, score_struct b){
	return a.score < b.score;
}

deque< score_struct > sc;

//Obstacle structure
typedef struct{
	float obs_x, obs_y;
	bool show;
	int frame;
	SDL_Rect obs;
}obstacle_struct;

deque< obstacle_struct > obstacles;
int obs_h, obs_w;

//Enemy structure
typedef struct{
	float enemy_x, enemy_y;
	bool show;
	int frame;
	int health;
	SDL_Rect enemy;
}enemy_struct;

deque < enemy_struct > enemies;
int enemy_h, enemy_w;

//Bullet structure
typedef struct{
	float bul_x, bul_y;
	bool show;
	SDL_Rect bul;
}bullet_struct;

deque< bullet_struct > bullets;
int bul_h, bul_w;

//Missile struct
typedef struct{
	float missile_x, missile_y;
	bool show;
	SDL_Rect missile;
}missile_struct;

missile_struct mis;

void initial_value(){

	score = 0;
	score_counter = 0;
	heli_x = WIDTH / 6.0f, heli_y = HEIGHT / 2.0f, speed = 0.0f;
	OBSTACLENUM = 5;
	ENEMYNUM = 5;
	TOTAL = 2;
	level = 0;
	obstacle_speed = 0.1f;
	mis.show = false;
	enemies.clear();
	bullets.clear();
	obstacles.clear();

}

void initialize(){

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0){
		cout << "SDL could not initialize. Error: " << SDL_GetError() << endl;
	}

	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
	TTF_Init();
	
	

	window = SDL_CreateWindow("game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
	if (window == NULL){
		cout << "Could not create window " << SDL_GetError() << endl;
		exit(1);
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	srand(time(NULL));

	//Make spawn points array
	int a = WIDTH + 100;
	for (int i = 0; i < 50; i++){
		x_axis[i] = a;
		a += 130;
	}

}

void score_input(){

	TTF_Font *font = TTF_OpenFont("gamefont.ttf", 30);
	SDL_Color color = { 0, 0, 0 };
	SDL_Rect input;
	input.x = 0;
	input.y = 0;
	image = IMG_Load("entername.png");
	SDL_Texture *input_name = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	SDL_QueryTexture(input_name, NULL, NULL, &input.w, &input.h);
	SDL_StartTextInput();
	SDL_Event score_event;
	string name = "";
	SDL_Rect name_rect;
	name_rect.x = 64;
	name_rect.y = 318;
	bool text = true;
	while (text){
		while (SDL_PollEvent(&score_event)){
			if (score_event.type == SDL_TEXTINPUT){
				name += score_event.text.text;
			}
			else if (score_event.type == SDL_KEYDOWN && score_event.key.keysym.sym == SDLK_BACKSPACE && name.size()){
				name.pop_back();

			}
			else if (score_event.type == SDL_KEYDOWN && score_event.key.keysym.sym == SDLK_ESCAPE){
				SDL_StopTextInput();
				score_struct temp;
				temp.score = score;
				temp.name = name;
				sc.push_back(temp);
				text = false;
			}
			else if (score_event.type == SDL_QUIT){
				text = false;
				play = false;
				running = false;
			}

			image = TTF_RenderText_Solid(font, name.c_str(), color);
			SDL_Texture *name_txt = SDL_CreateTextureFromSurface(renderer, image);
			SDL_FreeSurface(image);
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, input_name, NULL, &input);
			SDL_QueryTexture(name_txt, NULL, NULL, &name_rect.w, &name_rect.h);
			SDL_RenderCopy(renderer, name_txt, NULL, &name_rect);
			SDL_RenderPresent(renderer);
		}

	}
	sort(sc.begin(), sc.end(), sort_score);

}

void cap_fps(int fps, Uint32 starting_tick){
	if (1000.0f / fps > SDL_GetTicks() - starting_tick) {
		SDL_Delay(1000.0f / fps - (SDL_GetTicks() - starting_tick));
	}
}

int obstacle_distance_x = 100, obstacle_distance_y = 100;

bool check_enemy(enemy_struct temp){

	for (int i = 0; i < obstacles.size(); i++){
		if (SDL_HasIntersection(&temp.enemy, &obstacles[i].obs)) return true;
	}
	for (int i = 0; i < enemies.size(); i++){
		if (SDL_HasIntersection(&temp.enemy, &enemies[i].enemy)) return true;
	}

	return false;

}

bool check_obs(obstacle_struct temp){

	for (int i = 0; i < obstacles.size(); i++){
		if (SDL_HasIntersection(&temp.obs, &obstacles[i].obs)) return true;
	}
	for (int i = 0; i < enemies.size(); i++){
		if (SDL_HasIntersection(&temp.obs, &enemies[i].enemy)) return true;
	}

	return false;

}

void make_obstacle(){

	obstacle_struct temp;
	int x, y;
	do{
		temp.obs_x = x_axis[rand() % 50], temp.obs_y = y_axis[rand() % 4];
		temp.obs.x = temp.obs_x;
		temp.obs.y = temp.obs_y;
		temp.obs.w = obs_w;
		temp.obs.h = obs_h;
	} while (check_obs(temp));
	temp.show = true;
	temp.frame = 0;
	obstacles.push_back(temp);

}

void make_enemies(){

	enemy_struct temp;
	int x, y;
	do{
		temp.enemy_x = x_axis[rand() % 50], temp.enemy_y = y_axis[rand() % 4];
		temp.enemy.x = temp.enemy_x;
		temp.enemy.y = temp.enemy_y;
		temp.enemy.w = enemy_w;
		temp.enemy.h = enemy_h;
	} while (check_enemy(temp));
	temp.show = true;
	temp.frame = 0;
	temp.health = 5;
	enemies.push_back(temp);

}

void make_bullet(float heli_x, float heli_y){

	bullet_struct temp;
	temp.bul_x = heli_x + heli_w/2;
	temp.bul_y = heli_y + heli_h/2;
	temp.show = true;
	bullets.push_back(temp);

}

void load_heli(){

	image = IMG_Load("frame1.png");
	heli[0] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("frame2.png");
	heli[1] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("frame3.png");
	heli[2] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("frame4.png");
	heli[3] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("frame5.png");
	heli[4] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);

	SDL_QueryTexture(heli[0], NULL, NULL, &heli_w, &heli_h);

}

void load_obs(){

	image = IMG_Load("bomb1.png");
	bomb[0] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb2.png");
	bomb[1] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb3.png");
	bomb[2] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb4.png");
	bomb[3] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb5.png");
	bomb[4] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb6.png");
	bomb[5] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb7.png");
	bomb[6] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb8.png");
	bomb[7] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb9.png");
	bomb[8] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb10.png");
	bomb[9] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb11.png");
	bomb[10] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb12.png");
	bomb[11] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb13.png");
	bomb[12] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb14.png");
	bomb[13] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb15.png");
	bomb[14] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb16.png");
	bomb[15] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb17.png");
	bomb[16] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb18.png");
	bomb[17] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb19.png");
	bomb[18] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb20.png");
	bomb[19] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb21.png");
	bomb[20] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb22.png");
	bomb[21] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb23.png");
	bomb[22] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("bomb24.png");
	bomb[23] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);

	SDL_QueryTexture(bomb[0], NULL, NULL, &obs_w, &obs_h);

}

void load_enemy(){

	image = IMG_Load("enemy1.png");
	enemy[0] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("enemy2.png");
	enemy[1] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("enemy3.png");
	enemy[2] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("enemy4.png");
	enemy[3] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("enemy5.png");
	enemy[4] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);

	SDL_QueryTexture(enemy[0], NULL, NULL, &enemy_w, &enemy_h);

}

void menu(){
	
	TTF_Font *font = TTF_OpenFont("gamefont.ttf", 30);
	SDL_Color color = { 0, 0, 0 };

	SDL_Event menu_event;
	bool instruct = false;
	bool credit = false;
	bool score = false;
	image = IMG_Load("menu.png");
	SDL_Texture *background = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	SDL_Rect bg_rect;
	bg_rect.x = bg_rect.y = 0;
	SDL_QueryTexture(background, NULL, NULL, &bg_rect.w, &bg_rect.h);

	image = IMG_Load("credits.png");
	SDL_Texture *credits = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);

	image = IMG_Load("instructions.png");
	SDL_Texture *instructions = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);

	while (1){
		while (SDL_PollEvent(&menu_event)){

			if (menu_event.type == SDL_QUIT){
				running = false;
				return;
			}
			if (menu_event.type == SDL_MOUSEMOTION){
				int x = menu_event.motion.x;
				int y = menu_event.motion.y;
				if (x >= 492 && x <= 692 && y >= 178 && y <= 258) option = 0;
				if (x >= 268 && x <= 820 && y >= 266 && y <= 346) option = 1;
				if (x >= 310 && x <= 872 && y >= 356 && y <= 436) option = 2;
				if (x >= 428 && x <= 756 && y >= 446 && y <= 526) option = 3;
				if (x >= 492 && x <= 692 && y >= 538 && y <= 618) option = 4;
			}
			if (menu_event.type == SDL_MOUSEBUTTONDOWN){
				if (option == 0){
					initial_value();
					play = true;
					return;
				}
				if (option == 1){
					score = true;
				}
				if (option == 2){
					instruct = true;
				}
				if (option == 3){
					credit = true;
				}
				if (option == 4){
					play = false;
					running = false;
					return;
				}

			}

		}
		
		SDL_QueryTexture(heli[0], NULL, NULL, &heli_w, &heli_h);
		heli_rect.h = heli_h;
		heli_rect.w = heli_w;

		if (option == 0){
			heli_rect.x = 392;
			heli_rect.y = 178;
		}
		else if (option == 1){
			heli_rect.x = 268;
			heli_rect.y = 266;
		}
		else if (option == 2){
			heli_rect.x = 210;
			heli_rect.y = 356;
		}
		else if (option == 3){
			heli_rect.x = 328;
			heli_rect.y = 446;
		}
		else if (option == 4){
			heli_rect.x = 392;
			heli_rect.y = 538;
		}

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, background, NULL, &bg_rect);
		SDL_RenderCopy(renderer, heli[heli_frame], NULL, &heli_rect);
		heli_frame++;
		if (heli_frame == 5) heli_frame = 0;
		if (instruct){
			SDL_RenderCopy(renderer, instructions, NULL, &bg_rect);
		}
		if (credit){
			SDL_RenderCopy(renderer, credits, NULL, &bg_rect);
		}
		SDL_RenderPresent(renderer);
		if (instruct){
			SDL_Delay(5000);
			instruct = false;
		}
		if (credit){
			SDL_Delay(5000);
			credit = false;
		}

		if (score){
			image = IMG_Load("yellow.png");
			SDL_Texture *yellow = SDL_CreateTextureFromSurface(renderer, image);
			SDL_FreeSurface(image);
			SDL_Rect score_rect, name_rect;
			name_rect.x = name_rect.y = score_rect.y = 0;
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, yellow, NULL, &bg_rect);
			for (int i = 0; i < sc.size(); i++){
				string name = sc[i].name;
				image = TTF_RenderText_Solid(font, name.c_str(), color);
				SDL_Texture *nm = SDL_CreateTextureFromSurface(renderer, image);
				SDL_FreeSurface(image);
				long long score = sc[i].score;
				string sc;
				stringstream ss;
				ss << score;
				ss >> sc;
				SDL_QueryTexture(nm, NULL, NULL, &name_rect.w, &name_rect.h);
				score_rect.x = name_rect.x + name_rect.w + 100;
				image = TTF_RenderText_Solid(font, sc.c_str(), color);
				SDL_Texture *scr = SDL_CreateTextureFromSurface(renderer, image);
				SDL_FreeSurface(image);
				SDL_QueryTexture(scr, NULL, NULL, &score_rect.w, &score_rect.h);
				SDL_RenderCopy(renderer, nm, NULL, &name_rect);
				SDL_RenderCopy(renderer, scr, NULL, &score_rect);
				name_rect.y += 100;
				score_rect.y += 100;
			}
			SDL_RenderPresent(renderer);
			SDL_Delay(5000);
			score = false;
		}
		
		//cap_fps();

	}

}

void load_score(){
	FILE *fp;
	fp = fopen("highscore.txt", "rb");
	score_struct *temp;
	temp = (score_struct*)calloc(1, sizeof(score_struct));
	while (1){
		if (fread(temp, sizeof(score_struct), 1, fp)) sc.push_back(*temp);
		else break;
	}
	fclose(fp);
}

void store_score(){
	FILE *fp;
	fp = fopen("highscore.txt", "wb");
	for (int i = 0; i < sc.size(); i++){
		fwrite(&sc[i], sizeof(score_struct), 1, fp);
	}
	fclose(fp);
}

int main(int argc, char *argv[]){

	initialize();
	
	load_score();

	//Font
	TTF_Font *font = TTF_OpenFont("gamefont.ttf", 30);
	SDL_Color color = { 0, 0, 0 };
	SDL_Rect score_rect;
	score_rect.x = score_rect.y = 0;

	//Load background
	image = IMG_Load("bg.png");
	background[0] = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);

	SDL_Rect bg_rect;
	bg_rect.x = bg_rect.y = 0;
	SDL_QueryTexture(background[0], NULL, NULL, &bg_rect.w, &bg_rect.h);

	//Loading the heli
	load_heli();

	//Loading obstacle
	load_obs();

	//Loading enemy
	load_enemy();

	//Loading bullet
	image = IMG_Load("bullet.png");
	SDL_Texture *bul = SDL_CreateTextureFromSurface(renderer, image);
	SDL_QueryTexture(bul, NULL, NULL, &bul_w, &bul_h);
	SDL_FreeSurface(image);

	//Loading missile
	image = IMG_Load("missile.png");
	SDL_Texture *missile = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	int missile_h, missile_w;
	SDL_QueryTexture(missile, NULL, NULL, &missile_w, &missile_h);
	mis.show = false;

	//Load explosion
	image = IMG_Load("explosion.png");
	SDL_Texture *explosion = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	SDL_Rect exp;

	//Load gameover
	image = IMG_Load("gameover.png");
	SDL_Texture *gameover = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	SDL_Rect over;
	SDL_QueryTexture(gameover, NULL, NULL, &over.w, &over.h);
	over.x = WIDTH / 2 - 100;
	over.y = HEIGHT / 2 - 50;


	//Keyboard input variable
	const Uint8 *keyspace;
	const Uint8 *keyshoot;


	//Game Loop
	SDL_Event event;

	while (running){
		//Calling the menu function
		menu();

		auto previous_time = SDL_GetTicks(); 
		while (play){  

			//Stop drawing bullets when the go off screen
			for (int i = 0; i < bullets.size(); i++){
				if (bullets[i].show && bullets[i].bul_x > WIDTH){
					bullets[i].show = false;
					bullets.pop_front();
				}
			}

			//Stop drawing obstacles when they go off the screen
			for (int i = 0; i < obstacles.size(); i++){
				if (obstacles[i].show && obstacles[i].obs_x < 0){
					obstacles[i].show = false;
					obstacles.erase(obstacles.begin() + i);
				}
			}

			//Stop drawing enemies when they go off the screen
			for (int i = 0; i < enemies.size(); i++){
				if (enemies[i].show && enemies[i].enemy_x < 0){
					enemies[i].show = false;
					enemies.erase(enemies.begin() + i);
				}
			}

			//Make obstacles
			while (obstacles.size() < OBSTACLENUM){
				make_obstacle();
			}

			//Make enemies
			while (enemies.size() < ENEMYNUM){
				make_enemies();
			}

			//Make missile
			create_missile = rand() % 300;
			if (mis.show == false && create_missile == 50){
				mis.show = true;
				mis.missile_x = x_axis[rand() % 50];
				mis.missile_y = y_axis[rand() % 4];
				mis.missile.x = mis.missile_x;
				mis.missile.y = mis.missile_y;
				mis.missile.h = missile_h;
				mis.missile.w = missile_w;
			}

			//Update heli position in heli_rect
			heli_rect.x = heli_x;
			heli_rect.y = heli_y;
			heli_rect.w = heli_w;
			heli_rect.h = heli_h;

			while (SDL_PollEvent(&event)){

				if (event.type == SDL_QUIT){
					play = false;
					running = false;
				}

			}

			//Get keyboard input
			keyspace = SDL_GetKeyboardState(NULL);
			keyshoot = SDL_GetKeyboardState(NULL);

			//Make bullets if keyshoot is pressed
			if (keyshoot[SDL_SCANCODE_S] && bullets.size() < 13){

				if (bullets.size() == 0) make_bullet(heli_x, heli_y);
				else{
					bullet_struct temp = *(bullets.end() - 1);
					if (temp.bul_x - heli_x > 100) make_bullet(heli_x, heli_y);
				}
			}


			auto current_time = SDL_GetTicks(); 
			for (auto i = previous_time; i < current_time; i++){

				if (keyspace[SDL_SCANCODE_SPACE]){
					speed -= 0.002;
				}

				if (heli_y <= 60){
					heli_y = 60;
					speed = 0.00125;
				}
				if (heli_y > HEIGHT){
					play = false;
				}

				speed += 0.001;
				heli_y += speed;

				//Updating obstacles x co-ordinate
				for (int i = 0; i < obstacles.size(); i++){
					if (obstacles[i].show){
						obstacles[i].obs_x -= obstacle_speed;
						obstacles[i].obs.x = obstacles[i].obs_x;
						obstacles[i].obs.y = obstacles[i].obs_y;
						obstacles[i].obs.w = obs_w;
						obstacles[i].obs.h = obs_h;
					}
				}

				for (int i = 0; i < enemies.size(); i++){
					if (enemies[i].show){
						enemies[i].enemy_x -= obstacle_speed;
						enemies[i].enemy.x = enemies[i].enemy_x;
						enemies[i].enemy.y = enemies[i].enemy_y;
						enemies[i].enemy.w = enemy_w;
						enemies[i].enemy.h = enemy_h;
					}
				}

				//Updating co-ordinates of missile
				if (mis.show){
					mis.missile_x -= 0.8;
					mis.missile.x = mis.missile_x;
					mis.missile.y = mis.missile_y;
				}

				if (mis.show && mis.missile_x <= 0) mis.show = false;

				//Collision: heli with obstacles
				for (int i = 0; i < obstacles.size(); i++){
					if (SDL_HasIntersection(&heli_rect, &obstacles[i].obs)){
						obstacles.erase(obstacles.begin() + i);
						play = false;
					}
				}

				//Collision: heli with enemies
				for (int i = 0; i < enemies.size(); i++){
					if (SDL_HasIntersection(&heli_rect, &enemies[i].enemy)){
						enemies.erase(enemies.begin() + i);
						play = false;
					}
				}

				if (mis.show && SDL_HasIntersection(&heli_rect, &mis.missile)){
					play = false;
				}

				//Update bullets x co-ordinate
				for (int i = 0; i < bullets.size(); i++){
					if (bullets[i].show){
						bullets[i].bul_x += 0.3;
						bullets[i].bul.x = bullets[i].bul_x;
						bullets[i].bul.y = bullets[i].bul_y;
						bullets[i].bul.w = bul_w;
						bullets[i].bul.h = bul_h;
					}
				}

				//Collision: bullet with obstacles
				for (int i = 0; i < bullets.size(); i++){
					for (int j = 0; j < obstacles.size(); j++){
						if (SDL_HasIntersection(&bullets[i].bul, &obstacles[j].obs)){
							obstacles.erase(obstacles.begin() + j);
							bullets.erase(bullets.begin() + i);
							break;
						}
					}
				}

				//Collision: bullet with enemies
				for (int i = 0; i < bullets.size(); i++){
					for (int j = 0; j < enemies.size(); j++){
						if (SDL_HasIntersection(&bullets[i].bul, &enemies[j].enemy)){
							enemies[j].health--;
							if (enemies[j].health == 0) enemies.erase(enemies.begin() + j);
							bullets.erase(bullets.begin() + i);
							break;
						}
					}
				}

			}

			previous_time = current_time;

			//Clears screen
			SDL_RenderClear(renderer);

			//Draws stuff	
			SDL_RenderCopy(renderer, background[0], NULL, &bg_rect);
			bg_frame++;
			if (bg_frame == 80) bg_frame = 0;

			//Draw heli	
			if (!play){
				exp.x = heli_rect.x;
				exp.y = heli_rect.y;
				SDL_QueryTexture(explosion, NULL, NULL, &exp.w, &exp.h);
				SDL_RenderCopy(renderer, explosion, NULL, &exp);
				SDL_RenderCopy(renderer, gameover, NULL, &over);

			}
			else SDL_RenderCopyEx(renderer, heli[heli_frame], NULL, &heli_rect, speed * 18, NULL, SDL_FLIP_NONE);

			heli_frame++;
			if (heli_frame == 5) heli_frame = 0;

			//Draw bombs
			for (int i = 0; i < obstacles.size(); i++){
				if (obstacles[i].show){
					SDL_RenderCopyEx(renderer, bomb[obstacles[i].frame], NULL, &obstacles[i].obs, NULL, NULL, SDL_FLIP_HORIZONTAL);
					obstacles[i].frame++;
					if (obstacles[i].frame > 23) obstacles[i].frame = 0;
				}
			}

			//Draw enemies
			for (int i = 0; i < enemies.size(); i++){
				if (enemies[i].show){
					SDL_RenderCopyEx(renderer, enemy[enemies[i].frame], NULL, &enemies[i].enemy, NULL, NULL, SDL_FLIP_NONE);
					enemies[i].frame++;
					if (enemies[i].frame > 4) enemies[i].frame = 0;
				}
			}

			//Draw missile
			if (mis.show){
				SDL_RenderCopyEx(renderer, missile, NULL, &mis.missile, 0, NULL, SDL_FLIP_NONE);
			}

			//Draw bullets
			for (int i = 0; i < bullets.size(); i++){
				if (bullets[i].show) SDL_RenderCopyEx(renderer, bul, NULL, &bullets[i].bul, 0, NULL, SDL_FLIP_HORIZONTAL);
			}

			score_counter++;
			if (score_counter == 80){
				score++;
				score_counter = 0;
			}

			if (score % 10 == 0) obstacle_speed += .001;
			if (score % 30 == 0) level++;

			stringstream ss;
			ss << score;
			ss >> score_string;
			string print_score = "SCORE: ";
			print_score += score_string;
			cout << score << " " << score_string << endl;
			image = TTF_RenderText_Solid(font, print_score.c_str(), color);
			SDL_Texture *scr = SDL_CreateTextureFromSurface(renderer, image);
			SDL_FreeSurface(image);
			SDL_QueryTexture(scr, NULL, NULL, &score_rect.w, &score_rect.h);
			SDL_RenderCopy(renderer, scr, NULL, &score_rect);

			//Displays drawn stuff
			SDL_RenderPresent(renderer);

			if (!play){
				SDL_Delay(3000);
				if (sc.size() < 5) score_input();
				else{
					bool flag = false;
					for (int i = 0; i < sc.size(); i++){
						if (score>sc[i].score){
							sc.pop_front();
							flag = true;
							break;
						}
					}
					if (flag) score_input();

				}

				//Limit fps
				cap_fps(FPS, current_time);

			}

		}
	}



	//END
	//SDL_DestroyTexture(heli);
	//SDL_DestroyTexture(obs);
	

	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();

	return EXIT_SUCCESS;

}