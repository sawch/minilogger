# minilogger

A mini library to log data for a certain amount of time.

# usage
Here's an example of keeping a log of temperature for the past 72 hours:
```
int main()
{
	ml_set_desired_data_age(72 * 60 * 60);	/* 72 hours of data will be logged */
	ml_set_max_file_age(6 * 60 * 60);	/* each log file will contain 6 hours of data */
	ml_set_files_directory("/logs/temperature/");
	
	char log_buffer[64];
	while(1) {
		int len = sprintf(log_buffer, "%lu %lf\n", time(NULL), get_temperature());
		ml_push_to_file(log_buffer, sizeof(char), len);
		sleep(5 * 60);	/* log every 5 minutes */
	}
}
```

# notes

It will keep data for a little longer than you want, to avoid computation and file IO (like deleting entries and re-writing/seeking around one large log file).
It works by logging data into new files, deleting the old files when all of the data in them is older than the desired time range.

You'll always have at least your desired time range of data, it'll just go over it a little bit. Specifically, you will always have between `desired_data_age` and `desired_data_age` + `max_file_age` time worth of data logged.
A larger `max_file_age` results in the old logged data lasting longer, while smaller results in the data being split across more files.
