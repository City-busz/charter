#include "parser.h"

#include <string.h>
#include <stdlib.h>

#include "csv_parser/csvparser.h"

#define TOK_AXIS_X      "x-axis"    /*done*/
#define TOK_AXIS_Y      "y-axis"    /*done*/
#define TOK_PLOT        "plot"      /*done*/
/*later*/
#define TOK_SCATTER     "scatter" 
#define TOK_BAR         "bar"

#define TOK_LABEL       "label"     /*done*/

#define TOK_MODE        "mode"      /*done*/
#define TOK_MODE_LOG    "log"       /*done*/
#define TOK_MODE_LIN    "linear"    /*done*/

#define TOK_X_DATA      "x"
#define TOK_Y_DATA      "y"
/*later*/
#define TOK_COLOR       "color"
#define TOK_LIN_STYLE   "line-style"
#define TOK_LIN_WIDTH   "line-width"
#define TOK_MARKER      "marker"

#define TOK_RANGE       "range"     /*done*/

#define TOK_BEGIN       "("         /*done*/
#define TOK_END         ")"         /*done*/

#define TOK_WIDTH       "width"     
#define TOK_HEIGHT      "height"    
#define TOK_TITLE       "title"     


struct{
    double value;
    void* prev;
} typedef _dList;



void 
parse_mode(char *line, chart *chart, _pstate prev)
{
    unsigned int s = line[0] == ' ' ? 1 : 0;
    unsigned int n = strlen(line) - s;
    char * copy = malloc(n*sizeof(char));
    memcpy(copy, &line[s], n);
    if (strcmp(copy, TOK_MODE_LIN) == 0)
    {
        if (prev == AXIS_X) 
        {
            chart->x_axis.mode = LINEAR;
        } else
        {
            chart->y_axis.mode = LINEAR;
        }
    } else if (strcmp(copy, TOK_MODE_LOG)==0)
    {
        if (prev == AXIS_X) 
        {
            chart->x_axis.mode = LOG;
        } else
        {
            chart->y_axis.mode = LOG;
        }
    }
    free(copy);
}

void 
parse_range(char *line, chart *chart, _pstate prev)
{
    unsigned int n = strlen(line);
    char * copy = malloc(n*sizeof(char));
    memcpy(copy, line, n);
    char * tok = copy;
    char * point;
    unsigned int i = 0;
    while((tok = strtok_r(tok, " ,", &point)) != NULL)
    {
        double v = atof(tok);
        if (i == 0)
        {
            if (prev == AXIS_X)
            {
                chart->x_axis.autoscale = FALSE;
                chart->x_axis.range_min = v;
            } else
            {
                chart->y_axis.autoscale = FALSE;
                chart->y_axis.range_min = v;
            }
        } else if (i == 1)
        {
            if (prev == AXIS_X)
            {
                chart->x_axis.range_max = v;
            } else
            {
                chart->y_axis.range_max = v;
            }
        }
        i ++;
        tok = NULL;
    }
    free(copy);
}

cbool startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? FALSE : strncmp(pre, str, lenpre) == 0;
}

_dList* parse_csv(char * link, unsigned int * size)
{
    unsigned int n = strlen(link)-6;
    if (n <= 0)
    {
        return NULL;
    }
    char * copy = malloc(n*sizeof(char));
    memcpy(copy, &link[6], n);
    char *  tag;
    char * url = strtok_r(copy, "#", &tag);

    CsvParser *csvparser = CsvParser_new(url, ",", 1);
    CsvRow *header;
    CsvRow *row;
    header = CsvParser_getHeader(csvparser);
    if (header == NULL)
    {
        return NULL;
    }

    char **headerFields = CsvParser_getFields(header);
    int i;
    int id = -1;
    for (i = 0 ; i < CsvParser_getNumFields(header) ; i++) {
        if (strcmp(headerFields[i], tag) == 0){
            id  = i;
        }
    }

    if (id < 0)
    {
        return NULL;
    }
    _dList * list = NULL;
    while ((row = CsvParser_getRow(csvparser)) ) {
        char **rowFields = CsvParser_getFields(row);
        double val = atof(rowFields[id]);
        _dList *el = malloc(sizeof(_dList));
        el->value = val;
        el->prev = list;
        list = el;
        *size = *size + 1;
        CsvParser_destroy_row(row);
    }
    return list;
}


void  
parse_x_data(chart *chart, char* line)
{
    unsigned int n = strlen(line);
    char * local = malloc((1+n)*sizeof(char));
    memset(local, 0, n+1);
    memcpy(local, line, n);
    char * tok = local;
    char * point;
    
    unsigned int l = 0;
    _dList * list = NULL;
    /* count values */
    while((tok = strtok_r(tok, " ,)\n", &point)) != NULL)
    {
        if (startsWith("csv://", tok))
        {   
            list = parse_csv(tok, &l);
            break;
        }
        _dList *el = malloc(sizeof(_dList));
        el->value = atof(tok);
        el->prev = list;
        list = el;

        l ++;
        tok = NULL;
    }
    
    
    double * data = malloc(l*sizeof(double)); 
    int i;
    for (i = l-1 ; i >= 0 ; i--)
    {
        data[i] = list->value;
        list = list->prev;
    }

    plot_get_last_element(chart->plots)->plot->n = l;
    plot_get_last_element(chart->plots)->plot->x_data = data;
    free(local);
}

void  
parse_y_data(chart *chart, char* line)
{
    unsigned int n = strlen(line);
    char * copy = malloc((1+n)*sizeof(char));
    memset(copy, 0, n+1);
    memcpy(copy, line, n);
    char * tok = copy;
    char * point;
    
    unsigned int l = 0;
    _dList * list = NULL;
    /* count values */
    while((tok = strtok_r(tok, " ,", &point)) != NULL)
    {
        if (startsWith("csv://", tok))
        {   
            list = parse_csv(tok, &l);
            break;
        }
        _dList *el = malloc(sizeof(_dList));
        el->value = atof(tok);
        el->prev = list;

        list = el;
        l ++;
        tok = NULL;
    }
    
    double * data = malloc(l*sizeof(double)); 
    int i;
    for (i = l-1 ; i >= 0 ; i--)
    {
        data[i] = list->value;
        list = list->prev;
    }
    plot_get_last_element(chart->plots)->plot->n = l;
    plot_get_last_element(chart->plots)->plot->y_data = data;
    free(copy);
}



char * parse_text(char * rest)
{
    unsigned int i = rest[0] == ' '? 1 : 0;

    unsigned int n = strlen(rest) - i ;
    char * copy = malloc(n*sizeof(char));
    memset(copy, 0, n);
    memcpy(copy, &rest[i], n);
    return copy;
}

_pstate 
parse_line(char* line, chart * chart, _pstate prev)
{
    unsigned int n = strlen(line);
    char * copy = malloc(n*sizeof(char));
    memcpy(copy, line, n);
    char * tok = copy;
    char * rest;
    while((tok = strtok_r(tok, " (:", &rest)) != NULL)
    {
        if (strcmp(tok, TOK_AXIS_X) == 0)
        {
            prev = AXIS_X;
        } else if (strcmp(tok, TOK_END) == 0)
        {
            prev = NONE;
        } else if (strcmp(tok, TOK_AXIS_Y) ==0)
        {
            prev = AXIS_Y;
        } else if (strcmp(tok, TOK_LABEL) == 0)
        {
            char * label = parse_text(rest);
            if (prev == AXIS_X)
            {   
                chart->x_axis.label = label;
            }
            else if (prev == AXIS_Y)
            {
                chart->y_axis.label = label;
            }
            else if (prev == PLOT)
            {
                plotList * el = plot_get_last_element(chart->plots);
                if (el->plot == NULL){
                    plot * p = malloc(sizeof(plot));
                    p->type = LINE;
                    p->label = NULL;
                    p->n = 0;
                    p->x_data = NULL;
                    p->y_data = NULL;

                    el->plot = p;
                    chart->n_plots ++;
                }
                el->plot->label = label;
            }
            break;
        } else if (strcmp(tok, TOK_RANGE) == 0)
        {
            if (prev == AXIS_X || prev == AXIS_Y)
            {
                parse_range(rest, chart, prev);
            }
            break;
        } else if (strcmp(tok, TOK_PLOT) == 0)
        {
            prev = PLOT;
            
            plot * p = malloc(sizeof(plot));
            p->type = LINE;
            p->label = NULL;
            p->n = 0;
            p->x_data = NULL;
            p->y_data = NULL;

            chart_add_plot(chart, p);
        } else if (strcmp(tok, TOK_MODE) == 0)
        {
            if (prev == AXIS_X || prev == AXIS_Y)
            {
                parse_mode(rest, chart, prev);
            }
            break;
        } else if (strcmp(tok, TOK_X_DATA) == 0 && prev == PLOT)
        {
            parse_x_data(chart, rest);
            break;
        } else if (strcmp(tok, TOK_Y_DATA) == 0 && prev == PLOT)
        {
            parse_y_data(chart, rest);
            break;
        }
        tok = NULL;
    }
    free(copy);
    return prev;
}

chart * 
parse_chart(char *text)
{
    chart* nchart = initialize_empty_chart();
    unsigned int n = strlen(text);
    char * copy = malloc(n*sizeof(char));
    memccpy(copy, text, 0, n);
    char* tok_pointer;
    char* line = strtok_r(copy, "\n", &tok_pointer);
    _pstate state = NONE;
    while (line != NULL)
    {
        state = parse_line(line, nchart, state);
        line = strtok_r(NULL, "\n", &tok_pointer);
    }
    free(copy);
    return nchart;
}
