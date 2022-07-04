#include <string.h>
#include <sys/types.h> /* open */
#include <sys/stat.h> /* open */
#include <fcntl.h> /* O_RDWR, O_CREAT, O_TRUNC, O_WRONLY */
#include <unistd.h> /* close */
#include <sys/wait.h>
#include "so_stdio.h"

#define SIZE 4096

typedef struct _so_file {
	int fd; /* file descriptor */
	int eof;
	int error;
	unsigned char buffer[SIZE];
	int buff_pos; /* buffer position */
	int opening_mode; /* 1 - read, 2 - write */
	int bytes; /* number of bytes readed */
	int cursor;
	int child;
} SO_FILE;

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	SO_FILE *file = (SO_FILE *) malloc(sizeof(SO_FILE));

	if (file == NULL) {
		file->error = 1;
		free(file);
		return NULL;
	}

	if (strcmp(mode, "r") == 0) {
		file->fd = open(pathname, O_RDONLY);
		file->opening_mode = 1;
		if (file->fd < 0) {
			free(file);
			return NULL;
		}
	} else if (strcmp(mode, "r+") == 0) {
		file->fd = open(pathname, O_RDWR);
		file->opening_mode = 1;
		if (file->fd < 0) {
			free(file);
			return NULL;
		}
	} else if (strcmp(mode, "w") == 0) {
		file->fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		file->opening_mode = 2;
		if (file->fd < 0) {
			free(file);
			return NULL;
		}
	} else if (strcmp(mode, "w+") == 0) {
		file->fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0644);
		file->opening_mode = 2;
		if (file->fd < 0) {
			free(file);
			return NULL;
		}
	} else if (strcmp(mode, "a") == 0) {
		file->fd = open(pathname, O_APPEND | O_CREAT | O_WRONLY, 0644);
		file->opening_mode = 3;
		if (file->fd < 0) {
			free(file);
			return NULL;
		}
	} else if (strcmp(mode, "a+") == 0) {
		file->fd = open(pathname, O_APPEND | O_RDWR | O_CREAT, 0644);
		file->opening_mode = 3;
		if (file->fd < 0) {
			free(file);
			return NULL;
		}
	} else {
		free(file);
		return NULL;
	}

	file->eof = 0;
	file->error = 0;
	file->buff_pos = 0;
	file->bytes = 0;
	file->cursor = 0;
	file->opening_mode = 0;
	file->child = 0;

	return file;
}

int so_fclose(SO_FILE *stream)
{
	int rc;

	if (stream == NULL)
		return SO_EOF;

	/* for opening mode == write need so_fflush */
	if (stream->opening_mode == 2) {
		rc = so_fflush(stream);
		if (rc < 0) {
			stream->error = 1;
			free(stream);
			return SO_EOF;
		}
	}

	rc = close(stream->fd);
	if (rc < 0) {
		stream->error = 1;
		free(stream);
		return SO_EOF;
	}

	free(stream);

	return 0;
}

int so_fgetc(SO_FILE *stream)
{
	if (stream == NULL)
		return SO_EOF;

	if (stream->bytes == 0 || stream->buff_pos == stream->bytes) {

		ssize_t bytesRead = read(stream->fd, stream->buffer, SIZE);

		if (bytesRead == -1) {
			stream->error = 1;
			return SO_EOF;
		} else if (bytesRead == 0) {
			stream->eof = 1;
			return SO_EOF;
		}
		stream->bytes = bytesRead;
		stream->buff_pos = 0;
		stream->opening_mode = 1;
		stream->cursor++;

		return (int) stream->buffer[stream->buff_pos++];
	}

	if (stream->buff_pos == stream->bytes) {
		stream->eof = 1;
		return SO_EOF;
	}
	stream->cursor++;

	return (int) stream->buffer[stream->buff_pos++];
}

int so_fputc(int c, SO_FILE *stream)
{
	if (stream == NULL)
		return SO_EOF;

	if (stream->buff_pos == SIZE) {
		int rc;

		rc = so_fflush(stream);
		if (rc == -1) {
			stream->error = 1;
			return SO_EOF;
		}
	}

	stream->buffer[stream->buff_pos++] = c;
	stream->cursor++;
	stream->opening_mode = 2;

	return c;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int i, j, c;
	char *aux = (char *) ptr;

	if (stream == NULL)
		return SO_EOF;

	int elemsRead = 0;
	int crtAuxElem = 0;

	for (i = 0; i < nmemb; i++) {
		for (j = 0; j < size; j++) {

			c = so_fgetc(stream);

			/* if fgetc has error return 0 */
			if (stream->eof == 1)
				break;

			if (stream->error == 1)
				return 0;

			aux[crtAuxElem++] = c;
		}
		if (stream->eof == 1)
			break;

		elemsRead++;
	}
	stream->opening_mode = 1;

	return elemsRead;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int i, j;
	char *aux = (char *) ptr;

	if (stream == NULL)
		return SO_EOF;

	int elemsWritten = 0;
	int crtAuxElem = 0;

	for (i = 0; i < nmemb; i++) {
		for (j = 0; j < size; j++) {

			so_fputc(aux[crtAuxElem++], stream);

			if (stream->error == 1)
				return 0;
		}
		elemsWritten++;
	}
	stream->opening_mode = 2;

	return elemsWritten;
}

int so_fflush(SO_FILE *stream)
{
	int rc;

	if (stream == NULL)
		return SO_EOF;

	int charsWritten = 0;

	if (stream->opening_mode != 2)
		return SO_EOF;

	if (stream->opening_mode == 2) {
		rc = write(stream->fd, stream->buffer, stream->buff_pos);
		if (rc < 0) {
			stream->error = 1;
			stream->buff_pos = 0;
			return SO_EOF;
		}

		/* as long as the total number of characters has not been entered */
		charsWritten += rc;
		while (charsWritten < stream->buff_pos) {
			rc = write(stream->fd, &stream->buffer[charsWritten], stream->buff_pos - charsWritten);
			charsWritten += rc;
		}
	}

	stream->buff_pos = 0;

	return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	int rc, position;

	if (stream == NULL)
		return SO_EOF;

	if (stream->opening_mode == 1) {
		stream->buff_pos = 0;
		stream->bytes = 0;
	}

	if (stream->opening_mode == 2) {
		rc = so_fflush(stream);
		if (rc < 0)
			return SO_EOF;
	}

	position = lseek(stream->fd, offset, whence);
	if (position < 0)
		return -1;
	stream->cursor = position;

	return 0;
}

long so_ftell(SO_FILE *stream)
{
	if (stream->error == 1)
		return -1;

	return stream->cursor;
}

int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}

int so_feof(SO_FILE *stream)
{
	return stream->eof;
}

int so_ferror(SO_FILE *stream)
{
	return stream->error;
}

SO_FILE *so_popen(const char *command, const char *type)
{
	int filedes[2];
	int fd = 0;
	int pp = pipe(filedes);

	if (pp == -1)
		return NULL;

	pid_t pid = fork();

	switch (pid) {
	case -1:
		close(filedes[0]);
		close(filedes[1]);
		return NULL;
	case 0:
		if (strcmp(type, "r") == 0) {
			close(filedes[0]);
			dup2(filedes[1], STDOUT_FILENO);
			close(filedes[1]);
		} else if (strcmp(type, "w") == 0) {
			close(filedes[1]);
			dup2(filedes[0], STDIN_FILENO);
			close(filedes[0]);
		}
		execlp("sh", "sh", "-c", command, NULL);
		exit(1);
	default:
		if (strcmp(type, "r") == 0) {
			close(filedes[1]);
			fd = filedes[0];
		} else if (strcmp(type, "w") == 0) {
			close(filedes[0]);
			fd = filedes[1];
		}
	}

	SO_FILE *stream = (SO_FILE *) malloc(sizeof(SO_FILE));

	if (stream == NULL) {
		stream->error = 1;
		free(stream);
		return NULL;
	}

	stream->fd = fd;
	stream->buff_pos = 0;
	stream->eof = 0;
	stream->error = 0;
	stream->bytes = 0;
	stream->cursor = 0;
	stream->child = pid;

	return stream;
}

int so_pclose(SO_FILE *stream)
{
	int status;
	pid_t pid = stream->child;

	if (pid == -1)
		return SO_EOF;

	int close_process = so_fclose(stream);

	if (close_process == -1)
		return SO_EOF;

	int wait_process = waitpid(pid, &status, 0);

	if (wait_process == -1)
		return -1;

	return status;
}
