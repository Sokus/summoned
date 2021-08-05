#include "summoned_platform.h"

#include <windows.h>
#include <stdio.h>

#include "win32_platform.h"

global B32 global_running;
global B32 global_code_reload;
global Win32OffscreenBuffer global_backbuffer;
global I64 global_perf_count_frequency;
global B32 debug_global_show_cursor;
global WINDOWPLACEMENT global_window_position = { .length=sizeof(global_window_position) };

LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

F32
Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    F32 result;
    result = ((F32)(end.QuadPart - start.QuadPart) /
              (F32)global_perf_count_frequency);
    return result;
}

internal void
CatStrings(size_t source_a_count, char *source_a,
           size_t source_b_count, char *source_b,
           size_t dest_count, char *dest)
{
    // TODO(casey): Dest bounds checking!
    
    for(size_t index = 0;
        index < source_a_count;
        ++index)
    {
        *dest++ = *source_a++;
    }
    
    for(size_t index = 0;
        index < source_b_count;
        ++index)
    {
        *dest++ = *source_b++;
    }
    
    *dest = 0;
}

internal int
StringLength(char *string)
{
    int count = 0;
    while(*string++)
    {
        ++count;
    }
    
    return count;
}

internal void
Win32InitEXEPath(Win32State *state)
{
    DWORD size_of_filename = GetModuleFileNameA(0, state->exe_path, sizeof(state->exe_path));
    state->one_past_last_exe_path_slash = state->exe_path;
    for(char *scan = state->exe_path;
        *scan;
        ++scan)
    {
        if(*scan == '\\')
        {
            state->one_past_last_exe_path_slash = scan + 1;
        }
    }
}



internal void
Win32BuildEXEPathFilename(Win32State *state, char *filename, int dest_count, char *dest)
{
    size_t path_count = (size_t)(state->one_past_last_exe_path_slash - state->exe_path);
    CatStrings(path_count, state->exe_path,
               (size_t)StringLength(filename), filename,
               (size_t)dest_count, dest);
}

FILETIME
Win32GetLastWriteTime(char *filename)
{
    FILETIME result = {0};
    
    WIN32_FILE_ATTRIBUTE_DATA data;
    if(GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
    {
        result = data.ftLastWriteTime;
    }
    
    return result;
}

internal Win32GameCode
Win32LoadGameCode(char *source_dll_name, char *temp_dll_name)
{
    Win32GameCode result = {0};
    
    result.last_dll_write_time = Win32GetLastWriteTime(source_dll_name);
    
    CopyFile(source_dll_name, temp_dll_name, FALSE);
    
    result.game_code_dll = LoadLibraryA(temp_dll_name);
    if(result.game_code_dll)
    {
        result.UpdateAndRender = (GameUpdateAndRenderType *)
            GetProcAddress(result.game_code_dll, "GameUpdateAndRender");
        
        result.is_valid = (result.UpdateAndRender) != 0;
    }
    
    if(!result.is_valid)
    {
        result.UpdateAndRender = 0;
    }
    
    return result;
}

internal void
Win32UnloadGameCode(Win32GameCode *game_code)
{
    if(game_code->game_code_dll)
    {
        FreeLibrary(game_code->game_code_dll);
        game_code->game_code_dll = 0;
    }
    
    game_code->UpdateAndRender = 0;
    game_code->is_valid = FALSE;
}

internal void
Win32ResizeDIBSection(Win32OffscreenBuffer *buffer, int width, int height)
{
    // TODO(casey): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.
    if(buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }
    
    buffer->width = width;
    buffer->height = height;
    
    int bytes_per_pixel = 4;
    buffer->bytes_per_pixel = bytes_per_pixel;
    
    // NOTE(casey): When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left!
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;
    
    // NOTE(casey): Thank you to Chris Hecker of Spy Party fame
    // for clarifying the deal with StretchDIBits and BitBlt!
    // No more DC for us.
    int bitmap_memory_size = buffer->width * buffer->height * buffer->bytes_per_pixel;
    buffer->memory = VirtualAlloc(0, (size_t)bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = buffer->width * buffer->bytes_per_pixel;
    
    
    // TODO(casey): Probably clear this to black
}

internal Win32WindowDimension
Win32GetWindowDimension(HWND window)
{
    Win32WindowDimension result;
    
    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;
    
    return result;
}

internal void
Win32DisplayBufferInWindow(Win32OffscreenBuffer *buffer,
                           HDC device_context,
                           int window_width,
                           int window_height)
{
    // TODO(casey): Centering / black bars?
    
    if((window_width >= buffer->width*2) &&
       (window_height >= buffer->height*2))
    {
        ASSERT(!" ");
        StretchDIBits(device_context,
                      0, 0, 2*buffer->width, 2*buffer->height,
                      0, 0, buffer->width, buffer->height,
                      buffer->memory,
                      &buffer->info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        int offset_x = 10;
        int offset_y = 10;
        
        PatBlt(device_context, 0, 0, window_width, offset_y, BLACKNESS);
        PatBlt(device_context, 0, offset_y+buffer->height, window_width, window_height, BLACKNESS);
        PatBlt(device_context, 0, 0, offset_x, window_height, BLACKNESS);
        PatBlt(device_context, offset_x+buffer->width, 0, window_width, window_height, BLACKNESS);
        
        // NOTE(casey): For prototyping purposes, we're going to always blit
        // 1-to-1 pixels to make sure we don't introduce artifacts with
        // stretching while we are learning to code the renderer!
        StretchDIBits(device_context,
                      offset_x, offset_y, buffer->width, buffer->height,
                      0, 0, buffer->width, buffer->height,
                      buffer->memory,
                      &buffer->info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
}


internal LRESULT CALLBACK
Win32MainWindowCallback(HWND window,
                        UINT message,
                        WPARAM wparam,
                        LPARAM lparam)
{
    LRESULT result = 0;
    
    switch(message)
    {
        case WM_CLOSE:
        {
            // TODO(casey): Handle this with a message to the user?
            global_running = FALSE;
        } break;
        
        case WM_SETCURSOR:
        {
            if(debug_global_show_cursor)
            {
                result = DefWindowProcA(window, message, wparam, lparam);
            }
            else
            {
                SetCursor(0);
            }
        } break;
        
        case WM_ACTIVATEAPP:
        {
            if(wparam == 1)
            {
                SetLayeredWindowAttributes(window, RGB(0, 0, 0), 255, LWA_ALPHA);
            }
            else
            {
                SetLayeredWindowAttributes(window, RGB(0, 0, 0), 64, LWA_ALPHA);
            }
        } break;
        
        case WM_DESTROY:
        {
            // TODO(casey): Handle this as an error - recreate window?
            global_running = FALSE;
        } break;
#if 0
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            ASSERT(!"Keyboard input came in through a non-dispatch message!");
        } break;
#endif
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            Win32WindowDimension dimension = Win32GetWindowDimension(window);
            Win32DisplayBufferInWindow(&global_backbuffer, device_context,
                                       dimension.width, dimension.height);
            EndPaint(window, &paint);
        } break;
        
        default:
        {
            // OutputDebugStringA("default\n");
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }
    
    return result;
}

internal void
Win32ToggleFullscreen(HWND window)
{
    // NOTE(casey): This follows Raymond Chen's prescription
    // for fullscreen toggling, see:
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
    
    DWORD style = (DWORD)GetWindowLong(window, GWL_STYLE);
    if(style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO monitor_info = { .cbSize=sizeof(monitor_info) };
        if(GetWindowPlacement(window, &global_window_position) &&
           GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
        {
            SetWindowLong(window, GWL_STYLE, (I32)style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window, HWND_TOP,
                         monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(window, GWL_STYLE, (I32)style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &global_window_position);
        SetWindowPos(window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}


internal void
Win32ProcessPendingMessages()
{
    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        switch(message.message)
        {
            case WM_QUIT:
            {
                global_running = FALSE;
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                U32 vk_code = (U32)message.wParam;
                
                // NOTE(casey): Since we are comparing WasDown to IsDown,
                // we MUST use == and != to convert these bit tests to actualy
                // 0 or 1 value.
                B32 was_down = ((message.lParam & (1 << 30)) != 0);
                B32 is_down = ((message.lParam & (1 << 31)) == 0);
                if(was_down != is_down)
                {
                    if(vk_code == VK_ESCAPE)
                    {
                        global_running = FALSE;
                    }
                }
                
                if(is_down)
                {
                    B32 alt_key_was_down = (message.lParam & (1 << 29));
                    if((vk_code == VK_F4) && alt_key_was_down)
                    {
                        global_running = FALSE;
                    }
                    else if ((vk_code == VK_RETURN) && alt_key_was_down)
                    {
                        if(message.hwnd)
                        {
                            Win32ToggleFullscreen(message.hwnd);
                        }
                    }
                }
            } break;
            
            default:
            {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            } break;
        }
    }
}

int CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prev_instance,
        LPSTR commandline,
        int show_code)
{
    Win32State win32_state = {0};
    
    LARGE_INTEGER perf_count_frequency_result;
    QueryPerformanceFrequency(&perf_count_frequency_result);
    global_perf_count_frequency = perf_count_frequency_result.QuadPart;
    
    Win32InitEXEPath(&win32_state);
    
    char source_game_code_dll_path[MAX_PATH];
    Win32BuildEXEPathFilename(&win32_state, "summoned.dll",
                              sizeof(source_game_code_dll_path), source_game_code_dll_path);
    
    char temp_game_code_dll_path[MAX_PATH];
    Win32BuildEXEPathFilename(&win32_state, "summoned_temp.dll",
                              sizeof(temp_game_code_dll_path), temp_game_code_dll_path);
    
    
    // NOTE(casey): Set the Windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular.
    UINT desired_scheduler_ms = 1;
    B32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
    
#if SUMMONED_DEBUG
    debug_global_show_cursor = TRUE;
#endif
    WNDCLASSA window_class = {0};
    
    
    /* NOTE(casey): 1080p display mode is 1920x1080 -> Half of that is 960x540
       1920 -> 2048 = 2048-1920 -> 128 pixels
       1080 -> 2048 = 2048-1080 -> pixels 968
       1024 + 128 = 1152
    */
    Win32ResizeDIBSection(&global_backbuffer, 960, 540);
    
    window_class.style = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = Win32MainWindowCallback;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    // window_class.hIcon;
    window_class.lpszClassName = "SummonedWindowClass";
    
    if(RegisterClassA(&window_class))
    {
        
        HWND window = CreateWindowExA(0, // WS_EX_TOPMOST|WS_EX_LAYERED,
                                      window_class.lpszClassName,
                                      "Summoned",
                                      WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      0,
                                      0,
                                      instance,
                                      0);
        
        if(window)
        {
            
            // TODO(casey): How do we reliably query this on Windows?
            int monitor_refresh_hz = 60;
            HDC refresh_dc = GetDC(window);
            int win32_refresh_rate = GetDeviceCaps(refresh_dc, VREFRESH);
            ReleaseDC(window, refresh_dc);
            if(win32_refresh_rate > 1)
            {
                monitor_refresh_hz = win32_refresh_rate;
            }
            F32 game_update_hz = ((F32)monitor_refresh_hz / 2.0f);
            F32 target_seconds_per_frame = 1.0f / (F32)game_update_hz;
            
            
            global_running = TRUE;
#if SUMMONED_DEBUG
            LPVOID base_address = (LPVOID)TERABYTES(2);
#else
            LPVOID base_address = 0;
#endif
            
            GameMemory game_memory = {0};
            game_memory.permanent_storage_size = MEGABYTES(256);
            game_memory.transient_storage_size = GIGABYTES(1);
            
            // TODO(casey): Handle various memory footprints (USING
            // SYSTEM METRICS)
            
            // TODO(casey): Use MEM_LARGE_PAGES and
            // call adjust token privileges when not on Windows XP?
            
            // TODO(casey): TransientStorage needs to be broken up
            // into game transient and cache transient, and only the
            // former need be saved for state playback.
            win32_state.total_size = 
                game_memory.permanent_storage_size + game_memory.transient_storage_size;
            win32_state.game_memory_block = VirtualAlloc(base_address, (size_t)win32_state.total_size,
                                                         MEM_RESERVE|MEM_COMMIT,
                                                         PAGE_READWRITE);
            game_memory.permanent_storage = win32_state.game_memory_block;
            game_memory.transient_storage = ((U8 *)game_memory.permanent_storage +
                                             game_memory.permanent_storage_size);
            
            if(game_memory.permanent_storage && game_memory.transient_storage)
            {
                LARGE_INTEGER last_counter = Win32GetWallClock();
                LARGE_INTEGER flip_wall_clock = Win32GetWallClock();
                LARGE_INTEGER last_code_reload_check = Win32GetWallClock();
                
                Win32GameCode game = Win32LoadGameCode(source_game_code_dll_path,
                                                       temp_game_code_dll_path);
                
                while(global_running)
                {
                    if((Win32GetSecondsElapsed(last_code_reload_check, flip_wall_clock)) > 1)
                    {
                        last_code_reload_check = flip_wall_clock;
                        
                        FILETIME new_dll_write_time = Win32GetLastWriteTime(source_game_code_dll_path);
                        if(CompareFileTime(&new_dll_write_time, &game.last_dll_write_time) != 0)
                        {
                            Win32UnloadGameCode(&game);
                            game = Win32LoadGameCode(source_game_code_dll_path,
                                                     temp_game_code_dll_path);
                        }
                    }
                    Win32ProcessPendingMessages();
                    
                    GameOffscreenBuffer buffer = {0};
                    buffer.memory = global_backbuffer.memory;
                    buffer.width = global_backbuffer.width;
                    buffer.height = global_backbuffer.height;
                    buffer.pitch = global_backbuffer.pitch;
                    buffer.bytes_per_pixel = global_backbuffer.bytes_per_pixel;
                    
                    if(game.UpdateAndRender)
                    {
                        game.UpdateAndRender(&buffer);
                    }
                    
                    LARGE_INTEGER work_counter = Win32GetWallClock();
                    F32 work_seconds_elapsed = Win32GetSecondsElapsed(last_counter, work_counter);
                    
                    // TODO(casey): NOT TESTED YET! PROBABLY BUGGY!!!!!
                    F32 seconds_elapsed_for_frame = work_seconds_elapsed;
                    if(seconds_elapsed_for_frame < target_seconds_per_frame)
                    {
                        if(sleep_is_granular)
                        {
                            DWORD sleep_ms = (DWORD)(1000.0f *(target_seconds_per_frame -
                                                               seconds_elapsed_for_frame));
                            if(sleep_ms > 0)
                            {
                                Sleep(sleep_ms);
                            }
                            
                        }
                        
                        F32 test_seconds_elapsed_for_frame =
                            Win32GetSecondsElapsed(last_counter,
                                                   Win32GetWallClock());
                        if(test_seconds_elapsed_for_frame < target_seconds_per_frame)
                        {
                            // TODO(casey): LOG MISSED SLEEP HERE
                        }
                        
                        // NOTE(sokus): CPU BURNER
                        while(seconds_elapsed_for_frame < target_seconds_per_frame)
                        {
                            seconds_elapsed_for_frame =
                                Win32GetSecondsElapsed(last_counter,
                                                       Win32GetWallClock());
                        }
                    }
                    else
                    {
                        // TODO(casey): MISSED FRAME RATE!
                        // TODO(casey): Logging
                    }
                    
                    LARGE_INTEGER end_counter = Win32GetWallClock();
                    F32 ms_per_frame = 1000.0f*Win32GetSecondsElapsed(last_counter, end_counter);
                    last_counter = end_counter;
                    
                    Win32WindowDimension dimension = Win32GetWindowDimension(window);
                    HDC device_context = GetDC(window);
                    Win32DisplayBufferInWindow(&global_backbuffer, device_context,
                                               dimension.width, dimension.height);
                    ReleaseDC(window, device_context);
                    
                    flip_wall_clock = Win32GetWallClock();
                }
            }
            else
            {
                // TODO(casey): Logging
            }
        }
        else
        {
            // TODO(casey): Logging
        }
    }
    else
    {
        // TODO(casey): Logging
    }
    
    return 0;
}