#include <stdio.h>
#include "raylib.h"

int
main()
{
	// Initialization
	//---------------------------------------------------------
	int ScreenWidth = 800;
	int ScreenHeight = 450;
	
	InitWindow(ScreenWidth, ScreenHeight, "raylib");
	SetTargetFPS(60);
	
	while(!WindowShouldClose())
	{
		BeginDrawing();
		
		ClearBackground(WHITE);
		DrawFPS(10, 10);
		
		EndDrawing();
	}
	
	CloseWindow();
	return(0);
}