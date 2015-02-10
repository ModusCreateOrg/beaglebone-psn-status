/* 
 * File:   main.c
 * Author: jgarcia
 *
 * Created on February 3, 2015, 7:04 AM
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <gtk-3.0/gtk/gtk.h>


CURL *curl;
GtkWidget *window;
GtkWidget *image;

char *global_image;

time_t last_curl;
time_t current_tick;
int time_diff;


//char *url = "https://support.us.playstation.com/";

char *url = "file://status.html";

struct MemoryStruct {
    char *memory;       // Used to store the result from the 
    size_t size;
};
 
 
static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */ 
        fprintf(stdout,"not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}
 
// Inspired by: http://curl.haxx.se/libcurl/c/getinmemory.html
int get_psn_status() {
    fprintf(stdout,"get_psn_status()\n");
    int psn_status = 0;
    
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
    chunk.size = 0;    /* no data at this point */ 

    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */ 
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(curl_handle);

    /* check for errors */ 
    if (res != CURLE_OK) {
        fprintf(stdout, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        psn_status = -1;
    }
    else {
        fprintf(stdout,"got response from curl\n"); 
        char *output = (char *)chunk.memory;
        
        char *pointer = strstr(output, "PSN Status: ONLINE");
        
        if (pointer) {
            fprintf(stdout, "PSN is up!\n");
            psn_status = 1;   
        }
        else {
           fprintf(stdout,"PSN is down!\n");
	}

    }
 
    /* cleanup curl stuff */ 
    curl_easy_cleanup(curl_handle);

    if(chunk.memory) {
        free(chunk.memory);
    }

    curl_global_cleanup();

    return psn_status;
}
    
    
void show_window(int argc, char *argv[]) {
    fprintf(stdout,"show_window();\n"); 
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_resize(GTK_WINDOW(window), 400, 300);
    
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_title(GTK_WINDOW(window), "Fetching...");

    gtk_container_set_border_width(GTK_CONTAINER(window), 2);


    g_signal_connect_swapped(
        G_OBJECT(window), 
        "destroy", 
        G_CALLBACK(gtk_main_quit), 
        G_OBJECT(window)
    );

    gtk_widget_show_all(window);
    gtk_window_fullscreen(GTK_WINDOW(window));

    fprintf(stdout,"gtk_main();\n");
    gtk_main();
}

/*
 * -1 = DaFuq (network down) -> Show bsod.gif
 *  0 = Down
 *  1 = up
 *  2 = searching
 */
void update_window(int status) {

    char *imageFile;
    
    fprintf(stdout,"update_window();\n");
    fprintf(stdout,"Status: %i\n", status);
    
    switch (status) {
        case -1:
            imageFile = "bsod.gif";
            break;
        case 0:
            imageFile = "thumbs_down.gif";
            break;
        case 1:
            imageFile = "thumbs_up.gif";
            break;
        case 2:
            imageFile = "searching.gif";
            break;
        default:
            imageFile = "bsod.gif";
            
    }
         
    fprintf(stdout,"imageFile = %s\n", imageFile);
    if (global_image == NULL || strcmp(imageFile, global_image) != 0) {
        fprintf(stdout,"Creating new image to put into GTK\n");  

        GList *children, *iter;

        children = gtk_container_get_children(GTK_CONTAINER(window));

        for(iter = children; iter != NULL; iter = g_list_next(iter)) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }

        if (children != NULL) {
            g_list_free(children); 
        }

        fprintf(stdout,"New image: %s", imageFile);
        global_image = imageFile;

        image = gtk_image_new_from_file(imageFile);

        gtk_container_add(GTK_CONTAINER(window), image);

        gtk_window_set_title(GTK_WINDOW(window), (status == 1) ? "PSN is ONLINE" : "PSN is OFFLINE");
        gtk_widget_show_all(window);
    }

}



void my_main_quit(void *args) {
    fprintf(stdout,"Exiting!\n");
    exit(0);
}

gboolean thread_fn(void *args) {
    
    current_tick = time(NULL);
    
    if (! last_curl || (current_tick - last_curl) > 680){
        fprintf(stdout,"Woke up!\n");
	if (! last_curl) {
            last_curl = current_tick;
        }


        int psn_status = get_psn_status();

        last_curl = current_tick;
        
        update_window(psn_status);
        fprintf(stdout,"Sleeping...\n");          
    }
    
    return TRUE;
}

void show_window2( int argc, char *argv[]) {
    
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_resize(GTK_WINDOW(window), 230, 150);
    
    //gtk_container_set_border_width(GTK_CONTAINER(window), 2);

    g_signal_connect_swapped(G_OBJECT(window), "destroy",
          G_CALLBACK(gtk_main_quit), G_OBJECT(window));
    
    g_signal_connect_swapped(G_OBJECT(window), "destroy",
          G_CALLBACK(my_main_quit), G_OBJECT(window));

            
    image = gtk_image_new_from_file("searching.gif");
      
    gtk_container_add(GTK_CONTAINER(window), image);    
    g_idle_add((GSourceFunc) thread_fn, 0);
    
    gtk_widget_show_all(window);
    gtk_window_fullscreen(GTK_WINDOW(window));

    gtk_main();
//    gdk_threads_leave();

}

int main(int argc, char *argv[]) {
    show_window2(argc, argv);
    
    exit(0);
}
