//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "ac/route_finder.h"
#include <memory>
#include "ac/route_finder_impl.h"
#include "debug/out.h"

using AGS::Common::Bitmap;

class IRouteFinder 
{
public:
    virtual ~IRouteFinder() = default;

    virtual void init_pathfinder() = 0;
    virtual void shutdown_pathfinder() = 0;
    virtual void set_walkablearea(Bitmap *walkablearea) = 0;
    virtual int can_see_from(int x1, int y1, int x2, int y2) = 0;
    virtual void get_lastcpos(int &lastcx, int &lastcy) = 0;
    virtual void set_route_move_speed(int speed_x, int speed_y) = 0;
    virtual int find_route(short srcx, short srcy, short xx, short yy, Bitmap *onscreen, int movlst, int nocross = 0, int ignore_walls = 0) = 0;
    virtual void calculate_move_stage(MoveList * mlsp, int aaa) = 0;
    virtual void recalculate_move_speeds(MoveList *mlsp, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y) = 0;
};

class AGSRouteFinder : public IRouteFinder 
{
public:
    void init_pathfinder() override
    { 
        AGS::Engine::RouteFinder::init_pathfinder(); 
    }
    void shutdown_pathfinder() override
    { 
        AGS::Engine::RouteFinder::shutdown_pathfinder(); 
    }
    void set_walkablearea(Bitmap *walkablearea) override
    { 
        AGS::Engine::RouteFinder::set_walkablearea(walkablearea);
    }
    int can_see_from(int x1, int y1, int x2, int y2) override
    { 
        return AGS::Engine::RouteFinder::can_see_from(x1, y1, x2, y2); 
    }
    void get_lastcpos(int &lastcx, int &lastcy) override
    { 
        AGS::Engine::RouteFinder::get_lastcpos(lastcx, lastcy); 
    }
    void set_route_move_speed(int speed_x, int speed_y) override
    { 
        AGS::Engine::RouteFinder::set_route_move_speed(speed_x, speed_y); 
    }
    int find_route(short srcx, short srcy, short xx, short yy, Bitmap *onscreen, int movlst, int nocross = 0, int ignore_walls = 0) override
    { 
        return AGS::Engine::RouteFinder::find_route(srcx, srcy, xx, yy, onscreen, movlst, nocross, ignore_walls); 
    }
    void calculate_move_stage(MoveList * mlsp, int aaa) override
    { 
        AGS::Engine::RouteFinder::calculate_move_stage(mlsp, aaa); 
    }
    void recalculate_move_speeds(MoveList *mlsp, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y) override
    {
        AGS::Engine::RouteFinder::recalculate_move_speeds(mlsp, old_speed_x, old_speed_y, new_speed_x, new_speed_y);
    }
};


std::unique_ptr<IRouteFinder> route_finder_impl;

void init_pathfinder(GameDataVersion game_file_version)
{
    AGS::Common::Debug::Printf(AGS::Common::MessageType::kDbgMsg_Info, "Initialize path finder library");
    route_finder_impl.reset(new AGSRouteFinder());
    route_finder_impl->init_pathfinder();
}

void shutdown_pathfinder()
{
    if (route_finder_impl)
        route_finder_impl->shutdown_pathfinder();
}

void set_walkablearea(Bitmap *walkablearea)
{
    route_finder_impl->set_walkablearea(walkablearea);
}

int can_see_from(int x1, int y1, int x2, int y2)
{
    return route_finder_impl->can_see_from(x1, y1, x2, y2);
}

void get_lastcpos(int &lastcx, int &lastcy)
{
    route_finder_impl->get_lastcpos(lastcx, lastcy);
}

void set_route_move_speed(int speed_x, int speed_y)
{
    route_finder_impl->set_route_move_speed(speed_x, speed_y);
}

int find_route(short srcx, short srcy, short xx, short yy, Bitmap *onscreen, int movlst, int nocross, int ignore_walls)
{
    return route_finder_impl->find_route(srcx, srcy, xx, yy, onscreen, movlst, nocross, ignore_walls);
}

void calculate_move_stage(MoveList * mlsp, int aaa)
{
    route_finder_impl->calculate_move_stage(mlsp, aaa);
}

void recalculate_move_speeds(MoveList *mlsp, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y)
{
    route_finder_impl->recalculate_move_speeds(mlsp, old_speed_x, old_speed_y, new_speed_x, new_speed_y);
}
