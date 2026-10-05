#include "sdl_ui.h"

#include "agent.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Ui {
    SDL_Window* window=nullptr;
    SDL_Renderer* renderer=nullptr;
    TTF_Font* font=nullptr;
    Ui() {
        SDL_SetMainReady();
        if(SDL_Init(SDL_INIT_VIDEO)!=0) throw std::runtime_error(SDL_GetError());
        if(TTF_Init()!=0) throw std::runtime_error(TTF_GetError());
        window=SDL_CreateWindow("Project 2 — bomb delivery agent",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
                                1280,800,SDL_WINDOW_RESIZABLE);
        renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
        if(!window||!renderer) throw std::runtime_error(SDL_GetError());
        const char* paths[]={"C:/Windows/Fonts/consola.ttf","C:/Windows/Fonts/arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"};
        for(const char* path:paths) if(std::filesystem::exists(path)) { font=TTF_OpenFont(path,15); if(font)break; }
        if(!font) throw std::runtime_error("No usable TTF font found (DejaVu Sans, Liberation Sans, or Arial)");
    }
    ~Ui(){if(font)TTF_CloseFont(font);if(renderer)SDL_DestroyRenderer(renderer);if(window)SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();}
};
void set(SDL_Renderer*r,SDL_Color c){SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);}
void label(Ui&u,const std::string&s,int x,int y,SDL_Color fg={225,232,241,255}){
    SDL_Surface* surface=TTF_RenderUTF8_Blended(u.font,s.c_str(),fg);if(!surface)return;
    SDL_Texture* texture=SDL_CreateTextureFromSurface(u.renderer,surface);
    SDL_Rect dst{x,y,surface->w,surface->h};SDL_RenderCopy(u.renderer,texture,nullptr,&dst);
    SDL_DestroyTexture(texture);SDL_FreeSurface(surface);
}
void button(Ui&u,SDL_Rect rect,const char* textValue,bool active=false){
    set(u.renderer,active?SDL_Color{47,116,139,255}:SDL_Color{42,52,67,255});
    SDL_RenderFillRect(u.renderer,&rect);set(u.renderer,{78,94,115,255});SDL_RenderDrawRect(u.renderer,&rect);
    label(u,textValue,rect.x+12,rect.y+10);
}
struct Simulation {
    std::unique_ptr<Environment> env;
    std::unique_ptr<Agent> agent;
    int step=0;
    std::string lastAction="Ready";
    explicit Simulation(const std::string& path){reset(path);}
    void reset(const std::string& path){
        env=std::make_unique<Environment>();env->LoadFromFile(path);
        agent=std::make_unique<Agent>();agent->Initialize();step=0;lastAction="Ready";
    }
    void advance(){if(env->IsFinished()||step>=200)return;Action chosen=agent->Decide(*env);Action result=env->ExecuteAction(chosen);lastAction=result.description;++step;}
};
std::vector<std::string> getMaps(const std::string& dir){
    std::vector<std::string> files;
    for(const auto& e:std::filesystem::directory_iterator(dir))if(e.is_regular_file()&&e.path().extension()==".txt")files.push_back(e.path().string());
    std::sort(files.begin(),files.end());return files;
}
std::string selectMap(Ui&u,const std::vector<std::string>&maps){
    int selected=0;bool done=false,quit=false;
    while(!done&&!quit){SDL_Event e;while(SDL_PollEvent(&e)){
        if(e.type==SDL_QUIT)quit=true;
        if(e.type==SDL_KEYDOWN){if(e.key.keysym.sym==SDLK_ESCAPE)quit=true;
            if(e.key.keysym.sym==SDLK_DOWN)selected=std::min(selected+1,(int)maps.size()-1);
            if(e.key.keysym.sym==SDLK_UP)selected=std::max(0,selected-1);
            if(e.key.keysym.sym==SDLK_RETURN)done=true;}
        if(e.type==SDL_MOUSEBUTTONDOWN&&e.button.x<700&&e.button.y>=125&&e.button.y<125+(int)maps.size()*42){selected=std::clamp((e.button.y-125)/42,0,(int)maps.size()-1);if(e.button.clicks>1)done=true;}
    }
    set(u.renderer,{17,22,30,255});SDL_RenderClear(u.renderer);label(u,"PROJECT 2 | SELECT MAP",34,30,{115,211,196,255});
    label(u,"Choose a map, then press ENTER. ESC closes the application.",34,72);
    for(int i=0;i<(int)maps.size();i++){
        SDL_Rect r{34,120+i*42,650,36};set(u.renderer,i==selected?SDL_Color{47,116,139,255}:SDL_Color{42,52,67,255});SDL_RenderFillRect(u.renderer,&r);
        label(u,std::filesystem::path(maps[i]).filename().string(),r.x+12,r.y+9);
    }
    SDL_RenderPresent(u.renderer);SDL_Delay(16);
    }
    if(quit) return {};
    if(maps.empty()) throw std::runtime_error("No .txt maps found in "+std::filesystem::absolute("maps").string());
    return maps.at(selected);
}
void draw(Ui&u,Simulation&s,int scroll,bool running){
    int ww,wh;SDL_GetWindowSize(u.window,&ww,&wh);SDL_Renderer*r=u.renderer;
    set(r,{17,22,30,255});SDL_RenderClear(r);
    const int side=370,pad=26;int cell=std::max(18,std::min(64,(ww-side-2*pad)/s.env->GetSize()));
    int mapX=pad,mapY=std::max(pad,(wh-s.env->GetSize()*cell)/2);
    Position agent=s.env->GetAgentPosition(),bomb=s.env->GetBombPosition(),exit=s.env->GetExitPosition();
    Percept p=s.env->GetPercept();
    for(int y=0;y<s.env->GetSize();y++)for(int x=0;x<s.env->GetSize();x++){
        SDL_Rect cellRect{mapX+x*cell,mapY+y*cell,cell-2,cell-2};SDL_Color fill{34,43,55,255};
        if(p.bombSensed&&!s.env->IsBombPicked()&&abs(x-agent.x)<=2&&abs(y-agent.y)<=2)fill={63,75,76,255};
        if(s.env->HasWall(x,y))fill={80,87,98,255};
        if(exit.x==x&&exit.y==y)fill={54,157,115,255};
        if(!s.env->IsBombPicked()&&bomb.x==x&&bomb.y==y)fill={207,138,61,255};
        if(agent.x==x&&agent.y==y)fill={70,154,220,255};
        set(r,fill);SDL_RenderFillRect(r,&cellRect);set(r,{53,64,79,255});SDL_RenderDrawRect(r,&cellRect);
        std::string mark="";if(agent.x==x&&agent.y==y)mark="A";else if(!s.env->IsBombPicked()&&bomb.x==x&&bomb.y==y)mark="B";else if(exit.x==x&&exit.y==y)mark="E";else if(s.env->HasWall(x,y))mark="X";
        if(!mark.empty())label(u,mark,cellRect.x+cell/2-5,cellRect.y+cell/2-9);
    }
    int sx=ww-side;label(u,"PROJECT 2 | BACKWARD REASONING",sx,24,{115,211,196,255});
    label(u,"A agent   B bomb   E exit   X wall",sx,53);
    label(u,"STEP "+std::to_string(s.step)+" / 200   COST "+std::to_string(s.env->GetTotalCost()),sx,84);
    label(u,"GOAL: "+s.agent->GetCurrentGoal(),sx,111,{245,203,111,255});
    label(u,"LAST ACTION: "+s.lastAction,sx,139);
    std::string status=s.env->MissionSucceeded()?"MISSION SUCCEEDED":s.env->MissionFailed()?"MISSION FAILED":s.step>=200?"STEP LIMIT REACHED":s.env->IsBombPicked()?"CARRYING THE BOMB":"SEARCHING";
    label(u,status,sx,167,s.env->IsFinished()?SDL_Color{106,218,156,255}:SDL_Color{180,190,205,255});
    button(u,{sx,200,105,38},"STEP  SPACE");button(u,{sx+114,200,112,38},running?"PAUSE ENTER":"RUN ENTER",running);
    button(u,{sx+235,200,100,38},"RESET  R");
    button(u,{sx,242,140,34},"MAP MENU  M");
    label(u,"Decision history (UP/DOWN to scroll)",sx,286,{170,184,204,255});
    const auto& history=s.agent->GetHistory();int y=314;int end=std::max(0,(int)history.size()-scroll);int first=std::max(0,end-18);
    for(int i=first;i<end;i++){
        const auto&e=history[i];std::string line="S"+std::to_string(e.situation)+" ["+e.kind+"] ";
        if(!e.premises.empty()) line+=e.premises.front()+" -> ";
        line+=e.conclusion;
        if(line.size()>48) line=line.substr(0,45)+"...";
        label(u,line,sx,y,e.kind=="INF"?SDL_Color{139,197,232,255}:SDL_Color{196,205,218,255});
        y+=25;
    }
    SDL_RenderPresent(r);
}
}

int RunSdlApplication(const std::string& mapDirectory){
    Ui ui;const auto maps=getMaps(mapDirectory);if(maps.empty())throw std::runtime_error("No .txt maps found in "+mapDirectory);
    std::string path=selectMap(ui,maps);if(path.empty())return 0;
    while(!path.empty()) {
    Simulation sim(path);bool quit=false,running=false,returnToMaps=false;int scroll=0;Uint32 last=SDL_GetTicks();
    while(!quit&&!returnToMaps){SDL_Event e;while(SDL_PollEvent(&e)){
        if(e.type==SDL_QUIT)quit=true;
        if(e.type==SDL_KEYDOWN){auto k=e.key.keysym.sym;
            if(k==SDLK_ESCAPE)quit=true;
            if(k==SDLK_m){returnToMaps=true;running=false;}
            if(k==SDLK_SPACE)sim.advance();
            if(k==SDLK_RETURN)running=!running;
            if(k==SDLK_r){sim.reset(path);scroll=0;}
            if(k==SDLK_UP)scroll=std::min(scroll+1,(int)sim.agent->GetHistory().size());
            if(k==SDLK_DOWN)scroll=std::max(0,scroll-1);
        }
        if(e.type==SDL_MOUSEBUTTONDOWN){if(e.button.y>=242&&e.button.y<278){int w,h;SDL_GetWindowSize(ui.window,&w,&h);int sx=w-370;
                if(e.button.x>=sx&&e.button.x<sx+145)returnToMaps=true;
            }
            if(e.button.y>=200&&e.button.y<240){int w,h;SDL_GetWindowSize(ui.window,&w,&h);int sx=w-370;
                if(e.button.x>=sx&&e.button.x<sx+110)sim.advance();else if(e.button.x<sx+230)running=!running;else if(e.button.x<sx+345){sim.reset(path);scroll=0;}}
        }
    }
    Uint32 now=SDL_GetTicks();if(running&&now-last>=400){sim.advance();last=now;}
    if(!returnToMaps){draw(ui,sim,scroll,running);SDL_Delay(8);}
    }
    sim.agent->SaveHistoryToFile("agent_history.log");
    if(quit)return 0;
    path=selectMap(ui,maps);
    }
    return 0;
}
