
#setwd("~/Dropbox/proyecto")
#setwd("C:/Users/ikun/Dropbox/proyecto")

library(splitstackshape)
library(ggplot2)
library(maps)
library(igraph)
library(data.table)
library(dplyr)


###################################################################################
# AUX CODE
  
printf <- function(...) invisible(print(sprintf(...)))

PrintTime <- function (name, t0)
{
  # prints time since t0, to measure how much `name` took
  # t0 <- proc.time()
  # do.serious.shit(data)
  # PrintTime("Foo", t0)
  s <- (proc.time()-t0)[[3]];
  print(sprintf("%s took %gs\n", name,  s));
  s;
}

Cut2 <- function(x, breaks, r=NULL) 
{
  # makes boxes, and represents them by their mean
  # from http://stackoverflow.com/a/5916794/462087
  if (is.null(r))
    r <- range(x)
  
  b <- seq(r[1], r[2], length=2*breaks+1)
  brk <- b[0:breaks*2+1]
  mid <- b[1:breaks*2]
  brk[1] <- brk[1]-0.01
  k <- cut(x, breaks=brk, labels=FALSE)
  mid[k]
}


##################################################################################
# Data load

#    s4 tweet layout
#    ---------------
#     #  |  field          | explanation                        | type
#    ----|-----------------|------------------------------------|-----------
#     01 | timestamp       | seconds since Aug-01 2013          | integer
#     02 | lat             | latitude                           | double
#     03 | lon             | longitude                          | double
#     04 | user.id         | user id                            | integer
#     05 | user.name       | user name                          | character
#     06 | hashtags        | hashtags used (comma-separated)    | character
#     07 | mentions(id)    | users mentioned (by their "my_id") | character
#     08 | mentiones(name) | users_mentioned (by their name)    | character
#     09 | my_id           | new id given from 1 to |U|         | integer
#     10 | message         | tokenized message                  | character

ReadTweetsOLD <- function( raw.file = 'data/s5.201308.unsorted', 
                        rdata    = NULL, #'data/rdata/s4.201308',
                        breaks   = 4000, breaks.lat = NA, breaks.lon = NA) 
{ 
  if (!is.null(rdata) && file.exists(rdata)){
    load(rdata);
    printf("reading tweets from rdata");
    tweets
  } else {
    t0 <- proc.time();
    tweets <- data.frame( read.csv2( raw.file, sep="|", dec=".", header=F, stringsAsFactors=F ))
    
    # obtain the cells 
    tweets$boxlat <- Cut2(tweets$lat, if (is.numeric(breaks.lat)) breaks.lat else breaks);
    tweets$boxlon <- Cut2(tweets$lon, if (is.numeric(breaks.lon)) breaks.lat else breaks);
    
    save(tweets, file=rdata)
    PrintTime("reading the tweets file", t0);
    
    tweets
  }
}

ReadTweets <- function( raw.file = 'data/s5.201308.unsorted', breaks   = 4000, breaks.lat = NA, breaks.lon = NA) 
{ 
    tweets <- data.frame( read.csv2( raw.file, sep="|", dec=".", header=F, stringsAsFactors=F,
                                     colClasses=c('integer', 'numeric', 'numeric', 'integer',
                                                  'character', 'integer', rep('character', 4), 'integer')))

    names(tweets) <- c('timestamp', 'lat', 'lon', 'geonameid', 'provincia', 'userid', 'username', 'hashtags.c', 'mentions.c', 'mentiones.name.c', 'id')


    tweets
}


Users <- function (tweets)
{
    t0 <- proc.time();
    
    u <- unique(select(tweets, userid, username, id))
    
    u
}

#############################################################################
# graphs

# undirected, unweighted
ReadEdgelist <- function (el.file, as.igraph=T) 
{
  t0 <- proc.time()
  
  edgelist <- read.csv2(el.file, sep=" ", colClasses=c("integer","integer"))
  names(edgelist) <- c("a","b")
  
  x <- if (as.igraph) {
    simplify( graph.data.frame(edgelist, directed=F), 
              remove.multiple = T, 
              remove.loops = F );    
  } else {
    unique(edgelist)
  }
  
  PrintTime("G1 by closeness", t0)
  x
}

# linking users that have mutually mentioned each other.
# first we retrieve the mentions, who mentions whom.
#now we identify mutual mentions. probably not the best way, but i think this is the fastest that has come to my mind:
#(the idea is to revert the mentions graph, making the A of each edge be the B, and the B be the A; now, any edge that is repeated is a mutual mention. as an optimization, I will only revert the edges where a > b (see `mentionsA`) and compare with those where a < b (`mentionsB`).)
# G1ByMentions will return a dataframe if `as.igraph` is `F`, an igraph object if `T`.
G1ByMentions <- function(tweets, as.igraph=T)
{
  t0 <- proc.time()
  
  mentions <- unique(
    cSplit(
      indt      = select( filter( tweets, mentions.c != "") , my.id, mentions.c ),
      splitCols ="mentions.c", 
      sep       = ",", 
      direction = "long"
    )
  )
  names(mentions) <- c("a","b")
  
  # select only mutual links
  mentionsA <- rename( select ( filter(mentions, 
                                       a > b),
                                b, a),
                       b=a, a=b)
  mentionsB <- filter(mentions, a < b)
  mentionsX <- rbind(mentionsA, mentionsB)
  
  mutual.mentions <- mentionsX[duplicated(mentionsX),]
  
  if (as.igraph) {
    g1.by.mentions <- graph.data.frame(mutual.mentions, directed=F)
  } else {
    g1.by.mentions <- mutual.mentions
  }
  
  PrintTime("G1 by mutual mentions", t0)
  
  g1.by.mentions
}

G1ByHashtags <- function(tweets) 
{
  t0 <- proc.time()
  
  uh <- unique( 
    cSplit( # Split Concatenated Values into Separate Values
      indt = select( filter(tweets, hashtags.c != ""), my.id, hashtags.c),  # select user.id, hashtags.c, from tweets where hashtags.c not ""
      splitCols = "hashtags.c", 
      sep = ",", 
      direction = "long"
    )
  )
  #=> user1->hashtag1; user2->hashtag1; user1->hashtag2;... (unique)
  
  hashtag.collisions <- filter( merge(uh,uh,by="hashtags.c",allow.cartesian=T), 
                                my.id.x < my.id.y)
  
  
  g1.by.hashtag <- graph.data.frame( unique( select(hashtag.collisions, 
                                                    my.id.x, my.id.y) ), 
                                     directed=F);
  
  PrintTime("G1ByHashtags", t0);
  g1.by.hashtag
}

