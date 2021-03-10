#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <curl/curl.h>

typedef struct	s_memory
{
	char		*response;
	size_t		size;
}				t_memory;

char	*ft_strjoin(const char *str1, const char *str2)
{
	int		i;
	int		len;
	int		str1_len;
	char	*new_str;

	str1_len = strlen(str1);
	len = str1_len + strlen(str2) + 1;
	if (!(new_str = malloc(len * sizeof(*new_str))))
		return (NULL);
	i = 0;
	while (i < str1_len)
	{
		new_str[i] = str1[i];
		i++;
	}
	while (i < len)
	{
		new_str[i] = str2[i - str1_len];
		i++;
	}
	new_str[i] = '\0';
	return (new_str);
}

int	ft_handle_public_key(t_memory *mem)
{
	int		fd;
	char	*first_nl;
	char	*keyname;
	char	*file;
	char	*keycontent;

	first_nl = strchr(mem->response, '\n');
	keyname = strndup(mem->response, first_nl - mem->response);
	file = ft_strjoin("/etc/cybele/public_keys.d/", keyname);
	keycontent = strndup(first_nl + 1, strchr(first_nl + 1, '\n') - first_nl + 1);
	fd = open(file, O_WRONLY | O_CREAT, 0777); /* think of a better permission */
	if (fd == -1)
		return (-1);
	write(fd, keycontent, strlen(keycontent));
	close(fd);
	free(keyname);
	free(file);
	free(keycontent);
	return (0);
}

size_t	ft_handle_curl_result(char *data, size_t size, size_t nmemb, void *userp)
{
	t_memory		*mem;
	size_t			realsize;
	char			*ptr;

	mem = userp;
	realsize = size * nmemb;
	ptr = realloc(mem->response, mem->size + realsize + 1);
	if (ptr == NULL)
     return (0);
	mem->response = ptr;
	memcpy(&mem->response[mem->size], data, realsize);
	mem->size += realsize;
	mem->response[mem->size] = '\0';
	if (strchr(strchr(data, '\n'), '\n') != NULL)
		ft_handle_public_key(mem);
	return (realsize);
}

int		main(void)
{
	CURL				*curl;
	CURLcode			result;
	t_memory			chunk;
	struct curl_slist	*list;

	curl = curl_easy_init();
	if (curl == NULL)
	{
		fprintf(stderr, "Error\nFailed to initialize curl...\n");
		return (1);
	}
	chunk.response = NULL;
	chunk.size = 0;
	list = NULL;
	list = curl_slist_append(list, "Accept: text/plain");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3000/api/key/get/valfur03");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ft_handle_curl_result);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
	result = curl_easy_perform(curl);
	if (result != CURLE_OK)
		fprintf(stderr, "Error\nDownload problem: %s\n", curl_easy_strerror(result));
	//printf("OK:\n%s", chunk.response);
	curl_slist_free_all(list);
	curl_easy_cleanup(curl);
	return (0);
}
