#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <curl/curl.h>

typedef struct	s_memory
{
	int			cloned_keys;
	int			total_keys;
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

char	*ft_strdup_replace(char *str, char *to_free)
{
	char	*tmp;

	tmp = strdup(str);
	free(to_free);
	return (tmp);
}

int	ft_handle_public_key(t_memory *mem)
{
	int		fd;
	char	*first_nl;
	char	*keyname;
	char	*file;
	char	*keycontent;

	mem->total_keys++;
	first_nl = strchr(mem->response, '\n');
	keyname = strndup(mem->response, first_nl - mem->response);
	file = ft_strjoin("/etc/cybele/public_keys.d/", keyname);
	keycontent = strndup(first_nl + 1, strchr(first_nl + 1, '\n') - first_nl + 1);
	mem->response = ft_strdup_replace(strchr(first_nl + 1, '\n') + 1, mem->response);
	fd = open(file, O_RDONLY);
	if (fd != -1)
		return (0);
	mem->cloned_keys++;
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

char	*ft_strchr_NULL(char *str, int c)
{
	if (str == NULL)
		return (NULL);
	return (strchr(str, c));
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
	while (ft_strchr_NULL(strchr(mem->response, '\n'), '\n') != NULL)
		ft_handle_public_key(mem);
	return (realsize);
}

int		ft_clone_keys(int argc, char **argv)
{
	int					i;
	char				*route;
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
	chunk.cloned_keys = 0;
	chunk.total_keys = 0;
	list = NULL;
	list = curl_slist_append(list, "Accept: text/plain");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ft_handle_curl_result);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
	i = 0;
	while (i < argc)
	{
		chunk.response = NULL;
		chunk.size = 0;
		route = ft_strjoin("http://localhost:3000/api/key/get/", argv[0]);
		curl_easy_setopt(curl, CURLOPT_URL, route);
		result = curl_easy_perform(curl);
		if (result != CURLE_OK)
			fprintf(stderr, "Error\nDownload problem: %s\n", curl_easy_strerror(result));
		free(chunk.response);
		free(route);
		i++;
	}
	//printf("OK:\n%s", chunk.response);
	printf("DONE\n%d key%s cloned over %d\n", chunk.cloned_keys, chunk.cloned_keys > 1 ? "s" : "", chunk.total_keys);
	curl_slist_free_all(list);
	curl_easy_cleanup(curl);
	return (1);
}

int		ft_handle_command(int argc, char **argv)
{
	if (strcmp("clone", argv[0]) == 0)
		ft_clone_keys(argc - 1, argv + 1);
	else
		return (0);
	return (1);
}

int		main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("Error\nExpected at least 2 arguments\n");
		return (0);
	}
	ft_handle_command(argc - 1, argv + 1);
	return (0);
}
