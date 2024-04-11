#include "pch.h"

/*
* Maximize the terminal window when running this application as at least 40 rows and columns
* must be visible simultaneously for the simple character plot to work correctly.
* 
* Use Ctrl+C to stop the program gracefully.
*/

static int stop = 0;

static void 
signal_handler(int signal)
{
    stop = 1;
}

int main(int argc, char* argv[])
{
    t_ranging_sensor lidar;
    qsigaction_t action;
    t_error result;

#if defined(_WIN32)
    /* Enable virtual terminal processing in the console */
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode, written;

    GetConsoleMode(console, &mode);
    SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    /* Clear the screen, save the cursor position and hide the cursor */
    WriteConsoleW(console, L"\033[2J\0337\033[?25l", 12, &written, NULL);
#else
    /* Clear the screen, save the cursor position and hide the cursor */
    printf("\033c\033[s\033[?25l");
#endif

    /* Install a Ctrl+C handler */
    action.sa_handler = signal_handler;
    action.sa_flags = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    /* Open the RPLIDAR device on COM11 */
    result = rplidar_open("serial://localhost:11?baud='115200',word='8',parity='none',stop='1',flow='none',dsr='on'", RANGING_DISTANCE_LONG, &lidar);
    if (result >= 0)
    {
        while (!stop)
        {
            static t_ranging_measurement measurements[1000];
            static const t_timeout interval = { 0, 100000000, false };   /* 100 ms interval */
            t_int num_measurements;

            /* Read one LIDAR scan into the provided buffer */
            num_measurements = rplidar_read(lidar, RANGING_MEASUREMENT_MODE_NORMAL, 0.0, 2 * M_PI, measurements, ARRAY_LENGTH(measurements));
            if (num_measurements > 0)
            {
                static char plot[40][40];
                int i;

                const int rows = ARRAY_LENGTH(plot[0]);
                const int cols = ARRAY_LENGTH(plot);
                const t_double inv_max_distance = 1 / 2.0;

                /*--- Do a simple character plot of the LIDAR scan ---*/

                /* Compute the plot */
                memset(plot, ' ', sizeof(plot));

                for (i = 0; i < num_measurements; i++)
                {
                    int x = (int)(0.5 * rows * (1 + inv_max_distance * measurements[i].distance * cos(measurements[i].heading)));
                    int y = (int)(0.5 * cols * (1 + inv_max_distance * measurements[i].distance * sin(measurements[i].heading)));

                    if (x >= 0 && x < cols && y >= 0 && y < rows)
                    {
                        if (plot[x][y] == ' ')
                            plot[x][y] = '.';           /* use a '.' for one point at this location */
                        else if (plot[x][y] == '.')
                            plot[x][y] = 'o';           /* use a 'o' for two points at this location */
                        else if (plot[x][y] == 'o')
                            plot[x][y] = '*';           /* use a '*' for three points at this location */
                        else if (plot[x][y] == '*')
                            plot[x][y] = 'O';           /* use a 'O' for four points at this location */
                        else if (plot[x][y] == 'O')
                            plot[x][y] = '@';           /* use a '@' for five or more points at this location */
                    }
                }

                /* Home the cursor by restoring the cursor position */
#if defined(_WIN32)
                WriteConsoleW(console, L"\0338", 2, &written, NULL);
#else
                printf('\033u');
#endif
                
                /* Print the plot */
                for (i = 0; i < rows; i++)
                {
                    int j;

                    for (j = 0; j < cols; j++)
                        putchar(plot[j][i]);

                    putchar('\n');
                }

                /* Wait for the next time interval */
                qtimer_sleep(&interval);
            }
        }

        /* Close the RPLIDAR device */
        rplidar_close(lidar);
    }

    if (result < 0)
    {
        char message[256];
        msg_get_error_messageA(NULL, result, message, ARRAY_LENGTH(message));
        fprintf(stderr, "ERROR: Unable to read LIDAR. %s (result=%d)\n", message, result);
    }

#if defined(_WIN32)
    /* Restore the cursor */
    WriteConsoleW(console, L"\033[?25h", 6, &written, NULL);

    /* Restore original console mode */
    SetConsoleMode(console, mode);
#else
    /* Restore the cursor */
    printf("\033[?25h");
#endif

    return result;
}
