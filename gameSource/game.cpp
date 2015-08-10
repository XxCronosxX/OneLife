int versionNumber = 26;

// retain an older version number here if server is compatible
// with older client versions.
// Change this number (and number on server) if server has changed
// in a way that breaks old clients.
int accountHmacVersionNumber = 25;



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

//#define USE_MALLINFO

#ifdef USE_MALLINFO
#include <malloc.h>
#endif


#include "minorGems/graphics/Color.h"





#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/CustomRandomSource.h"

#include "minorGems/system/Time.h"


// static seed
CustomRandomSource randSource( 34957197 );



#include "minorGems/util/log/AppLog.h"



#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/diffBundle/client/diffBundleClient.h"


#include "minorGems/formats/encodingUtils.h"



#include "spriteBank.h"
#include "objectBank.h"









// position of view in world
doublePair lastScreenViewCenter = {0, 0 };



// world with of one view
double viewWidth = 666;

// fraction of viewWidth visible vertically (aspect ratio)
double viewHeightFraction;

int screenW, screenH;

char initDone = false;

float mouseSpeed;

int musicOff;
float musicLoudness;

int webRetrySeconds;


double frameRateFactor = 1;
int baseFramesPerSecond = 60;

char firstDrawFrameCalled = false;
int firstServerMessagesReceived = 0;


char upKey = 'w';
char leftKey = 'a';
char downKey = 's';
char rightKey = 'd';




char *serverAddress = NULL;
int serverPort;

int serverSocket = -1;

int numServerBytesRead = 0;
int numServerBytesSent = 0;

int overheadServerBytesSent = 0;
int overheadServerBytesRead = 0;


int mapD = 32;

int *map;

int mapOffsetX = 0;
int mapOffsetY = 0;


char eKeyDown = false;



char doesOverrideGameImageSize() {
    return true;
    }



void getGameImageSize( int *outWidth, int *outHeight ) {
    *outWidth = 666;
    *outHeight = 666;
    }



const char *getWindowTitle() {
    return "One Dollar One Hour One Life";
    }


const char *getAppName() {
    return "OneLife";
    }

const char *getLinuxAppName() {
    // no dir-name conflict here because we're using all caps for app name
    return "OneLifeApp";
    }



const char *getFontTGAFileName() {
    return "font_32_64.tga";
    }


char isDemoMode() {
    return false;
    }


const char *getDemoCodeSharedSecret() {
    return "fundamental_right";
    }


const char *getDemoCodeServerURL() {
    return "http://FIXME/demoServer/server.php";
    }



char gamePlayingBack = false;


Font *mainFont;
Font *mainFontFixed;
Font *numbersFontFixed;

char *shutdownMessage = NULL;








static char wasPaused = false;
static float pauseScreenFade = 0;

static char *currentUserTypedMessage = NULL;



// for delete key repeat during message typing
static int holdDeleteKeySteps = -1;
static int stepsBetweenDeleteRepeat;


// if user clicks to initiate an action while still moving, we
// queue it here
static char *nextActionMessageToSend = NULL;

// block move until next PLAYER_UPDATE received after action sent
static char playerActionPending = false;
static int playerActionTargetX, playerActionTargetY;



#define SETTINGS_HASH_SALT "another_loss"


static const char *customDataFormatWriteString = 
    "version%d_mouseSpeed%f_musicOff%d_musicLoudness%f"
    "_webRetrySeconds%d";

static const char *customDataFormatReadString = 
    "version%d_mouseSpeed%f_musicOff%d_musicLoudness%f"
    "_webRetrySeconds%d";


char *getCustomRecordedGameData() {    
    
    float mouseSpeedSetting = 
        SettingsManager::getFloatSetting( "mouseSpeed", 1.0f );
    int musicOffSetting = 
        SettingsManager::getIntSetting( "musicOff", 0 );
    float musicLoudnessSetting = 
        SettingsManager::getFloatSetting( "musicLoudness", 1.0f );
    int webRetrySecondsSetting = 
        SettingsManager::getIntSetting( "webRetrySeconds", 10 );
    

    char * result = autoSprintf(
        customDataFormatWriteString,
        versionNumber, mouseSpeedSetting, musicOffSetting, 
        musicLoudnessSetting,
        webRetrySecondsSetting );
    

    return result;
    }



char showMouseDuringPlayback() {
    // since we rely on the system mouse pointer during the game (and don't
    // draw our own pointer), we need to see the recorded pointer position
    // to make sense of game playback
    return true;
    }



char *getHashSalt() {
    return stringDuplicate( SETTINGS_HASH_SALT );
    }




void initDrawString( int inWidth, int inHeight ) {
    mainFont = new Font( getFontTGAFileName(), 6, 16, false, 16 );
    mainFont->setMinimumPositionPrecision( 1 );

    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;

    // square window for this game
    viewWidth = 666 * 1.0 / viewHeightFraction;
    
    
    setViewSize( viewWidth );
    }


void freeDrawString() {
    delete mainFont;
    }



void initFrameDrawer( int inWidth, int inHeight, int inTargetFrameRate,
                      const char *inCustomRecordedGameData,
                      char inPlayingBack ) {
    
    gamePlayingBack = inPlayingBack;
    
    screenW = inWidth;
    screenH = inHeight;
    
    if( inTargetFrameRate != baseFramesPerSecond ) {
        frameRateFactor = 
            (double)baseFramesPerSecond / (double)inTargetFrameRate;
        }
    
    


    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;

    
    // square window for this game
    viewWidth = 666 * 1.0 / viewHeightFraction;
    
    
    setViewSize( viewWidth );


    
    

    

    setCursorVisible( true );
    grabInput( false );
    
    // world coordinates
    setMouseReportingMode( true );
    
    
    
    
    mainFontFixed = new Font( getFontTGAFileName(), 6, 16, true, 16 );
    numbersFontFixed = new Font( getFontTGAFileName(), 6, 16, true, 16, 16 );
    
    mainFontFixed->setMinimumPositionPrecision( 1 );
    numbersFontFixed->setMinimumPositionPrecision( 1 );
    

    float mouseSpeedSetting = 1.0f;
    
    int musicOffSetting = 0;
    float musicLoudnessSetting = 1.0f;
    int webRetrySecondsSetting = 10;

    
    int readVersionNumber;
    
    int numRead = sscanf( inCustomRecordedGameData, 
                          customDataFormatReadString, 
                          &readVersionNumber,
                          &mouseSpeedSetting, 
                          &musicOffSetting,
                          &musicLoudnessSetting,
                          &webRetrySecondsSetting );
    if( numRead != 5 ) {
        // no recorded game?
        }
    else {

        if( readVersionNumber != versionNumber ) {
            AppLog::printOutNextMessage();
            AppLog::warningF( 
                "WARNING:  version number in playback file is %d "
                "but game version is %d...",
                readVersionNumber, versionNumber );
            }
        }
    
    

    
    double mouseParam = 0.000976562;

    mouseParam *= mouseSpeedSetting;

    mouseSpeed = mouseParam * inWidth / viewWidth;

    musicOff = musicOffSetting;
    musicLoudness = musicLoudnessSetting;
    webRetrySeconds = webRetrySecondsSetting;

    
    serverAddress = SettingsManager::getStringSetting( "serverAddress" );

    if( serverAddress == NULL ) {
        serverAddress = stringDuplicate( "127.0.0.1" );
        }

    serverPort = SettingsManager::getIntSetting( "serverPort", 5077 );



    setSoundLoudness( musicLoudness );
    setSoundPlaying( false );


    map = new int[ mapD * mapD ];

    for( int i=0; i<mapD *mapD; i++ ) {
        map[i] = 0;
        }
    
    
    initSpriteBank();
    initObjectBank();
    

    initDone = true;
    }



void freeFrameDrawer() {

    printf( "Total received = %d bytes (+%d in headers), "
            "total sent = %d bytes (+%d in headers)\n",
            numServerBytesRead, overheadServerBytesRead,
            numServerBytesSent, overheadServerBytesSent );
    
    delete mainFontFixed;
    delete numbersFontFixed;
    
    if( currentUserTypedMessage != NULL ) {
        delete [] currentUserTypedMessage;
        currentUserTypedMessage = NULL;
        }

    

    if( shutdownMessage != NULL ) {
        delete [] shutdownMessage;
        shutdownMessage = NULL;
        }


    if( serverAddress != NULL ) {    
        delete [] serverAddress;
        serverAddress = NULL;
        }
    
    if( serverSocket != -1 ) {
        closeSocket( serverSocket );
        }


    freeObjectBank();
    freeSpriteBank();

    delete [] map;

    delete [] nextActionMessageToSend;
    }





    


// draw code separated from updates
// some updates are still embedded in draw code, so pass a switch to 
// turn them off
static void drawFrameNoUpdate( char inUpdate );




static void drawPauseScreen() {

    double viewHeight = viewHeightFraction * viewWidth;

    setDrawColor( 1, 1, 1, 0.5 * pauseScreenFade );
        
    drawSquare( lastScreenViewCenter, 1.05 * ( viewHeight / 3 ) );
        

    setDrawColor( 0.2, 0.2, 0.2, 0.85 * pauseScreenFade  );
        
    drawSquare( lastScreenViewCenter, viewHeight / 3 );
        

    setDrawColor( 1, 1, 1, pauseScreenFade );

    doublePair messagePos = lastScreenViewCenter;

    messagePos.y += 4.5  * (viewHeight / 15);

    mainFont->drawString( translate( "pauseMessage1" ), 
                           messagePos, alignCenter );
        
    messagePos.y -= 1.25 * (viewHeight / 15);
    mainFont->drawString( translate( "pauseMessage2" ), 
                           messagePos, alignCenter );

    if( currentUserTypedMessage != NULL ) {
            
        messagePos.y -= 1.25 * (viewHeight / 15);
            
        double maxWidth = 0.95 * ( viewHeight / 1.5 );
            
        int maxLines = 9;

        SimpleVector<char *> *tokens = 
            tokenizeString( currentUserTypedMessage );


        // collect all lines before drawing them
        SimpleVector<char *> lines;
        
            
        while( tokens->size() > 0 ) {

            // build up a a line

            // always take at least first token, even if it is too long
            char *currentLineString = 
                stringDuplicate( *( tokens->getElement( 0 ) ) );
                
            delete [] *( tokens->getElement( 0 ) );
            tokens->deleteElement( 0 );
            
            

            

            
            char nextTokenIsFileSeparator = false;
                
            char *nextLongerString = NULL;
                
            if( tokens->size() > 0 ) {

                char *nextToken = *( tokens->getElement( 0 ) );
                
                if( nextToken[0] == 28 ) {
                    nextTokenIsFileSeparator = true;
                    }
                else {
                    nextLongerString =
                        autoSprintf( "%s %s ",
                                     currentLineString,
                                     *( tokens->getElement( 0 ) ) );
                    }
                
                }
                
            while( !nextTokenIsFileSeparator 
                   &&
                   nextLongerString != NULL 
                   && 
                   mainFont->measureString( nextLongerString ) 
                   < maxWidth 
                   &&
                   tokens->size() > 0 ) {
                    
                delete [] currentLineString;
                    
                currentLineString = nextLongerString;
                    
                nextLongerString = NULL;
                    
                // token consumed
                delete [] *( tokens->getElement( 0 ) );
                tokens->deleteElement( 0 );
                    
                if( tokens->size() > 0 ) {
                    
                    char *nextToken = *( tokens->getElement( 0 ) );
                
                    if( nextToken[0] == 28 ) {
                        nextTokenIsFileSeparator = true;
                        }
                    else {
                        nextLongerString =
                            autoSprintf( "%s%s ",
                                         currentLineString,
                                         *( tokens->getElement( 0 ) ) );
                        }
                    }
                }
                
            if( nextLongerString != NULL ) {    
                delete [] nextLongerString;
                }
                
            while( mainFont->measureString( currentLineString ) > 
                   maxWidth ) {
                    
                // single token that is too long by itself
                // simply trim it and discard part of it 
                // (user typing nonsense anyway)
                    
                currentLineString[ strlen( currentLineString ) - 1 ] =
                    '\0';
                }
                
            if( currentLineString[ strlen( currentLineString ) - 1 ] 
                == ' ' ) {
                // trim last bit of whitespace
                currentLineString[ strlen( currentLineString ) - 1 ] = 
                    '\0';
                }

                
            lines.push_back( currentLineString );

            
            if( nextTokenIsFileSeparator ) {
                // file separator

                // put a paragraph separator in
                lines.push_back( stringDuplicate( "---" ) );

                // token consumed
                delete [] *( tokens->getElement( 0 ) );
                tokens->deleteElement( 0 );
                }
            }   


        // all tokens deleted above
        delete tokens;


        double messageLineSpacing = 0.625 * (viewHeight / 15);
        
        int numLinesToSkip = lines.size() - maxLines;

        if( numLinesToSkip < 0 ) {
            numLinesToSkip = 0;
            }
        
        
        for( int i=0; i<numLinesToSkip-1; i++ ) {
            char *currentLineString = *( lines.getElement( i ) );
            delete [] currentLineString;
            }
        
        int lastSkipLine = numLinesToSkip - 1;

        if( lastSkipLine >= 0 ) {
            
            char *currentLineString = *( lines.getElement( lastSkipLine ) );

            // draw above and faded out somewhat

            doublePair lastSkipLinePos = messagePos;
            
            lastSkipLinePos.y += messageLineSpacing;

            setDrawColor( 1, 1, 0.5, 0.125 * pauseScreenFade );

            mainFont->drawString( currentLineString, 
                                   lastSkipLinePos, alignCenter );

            
            delete [] currentLineString;
            }
        

        setDrawColor( 1, 1, 0.5, pauseScreenFade );

        for( int i=numLinesToSkip; i<lines.size(); i++ ) {
            char *currentLineString = *( lines.getElement( i ) );
            
            if( false && lastSkipLine >= 0 ) {
            
                if( i == numLinesToSkip ) {
                    // next to last
                    setDrawColor( 1, 1, 0.5, 0.25 * pauseScreenFade );
                    }
                else if( i == numLinesToSkip + 1 ) {
                    // next after that
                    setDrawColor( 1, 1, 0.5, 0.5 * pauseScreenFade );
                    }
                else if( i == numLinesToSkip + 2 ) {
                    // rest are full fade
                    setDrawColor( 1, 1, 0.5, pauseScreenFade );
                    }
                }
            
            mainFont->drawString( currentLineString, 
                                   messagePos, alignCenter );

            delete [] currentLineString;
                
            messagePos.y -= messageLineSpacing;
            }
        }
        
        

    setDrawColor( 1, 1, 1, pauseScreenFade );

    messagePos = lastScreenViewCenter;

    messagePos.y -= 3.75 * ( viewHeight / 15 );
    //mainFont->drawString( translate( "pauseMessage3" ), 
    //                      messagePos, alignCenter );

    messagePos.y -= 0.625 * (viewHeight / 15);

    const char* quitMessageKey = "pauseMessage3";
    
    if( isQuittingBlocked() ) {
        quitMessageKey = "pauseMessage3b";
        }

    mainFont->drawString( translate( quitMessageKey ), 
                          messagePos, alignCenter );

    }



void deleteCharFromUserTypedMessage() {
    if( currentUserTypedMessage != NULL ) {
                    
        int length = strlen( currentUserTypedMessage );
        
        char fileSeparatorDeleted = false;
        if( length > 2 ) {
            if( currentUserTypedMessage[ length - 2 ] == 28 ) {
                // file separator with spaces around it
                // delete whole thing with one keypress
                currentUserTypedMessage[ length - 3 ] = '\0';
                fileSeparatorDeleted = true;
                }
            }
        if( !fileSeparatorDeleted && length > 0 ) {
            currentUserTypedMessage[ length - 1 ] = '\0';
            }
        }
    }




SimpleVector<unsigned char> serverSocketBuffer;


// reads all waiting data from socket and stores it in buffer
void readServerSocketFull() {

    unsigned char buffer[512];
    
    int numRead = readFromSocket( serverSocket, buffer, 512 );
    
    
    while( numRead > 0 ) {
        serverSocketBuffer.appendArray( buffer, numRead );
        numServerBytesRead += numRead;

        numRead = readFromSocket( serverSocket, buffer, 512 );
        }    
    }



// NULL if there's no full message available
char *getNextServerMessage() {
    // find first terminal character #

    int index = serverSocketBuffer.getElementIndex( '#' );
        
    if( index == -1 ) {
        return NULL;
        }
    
    char *message = new char[ index + 1 ];
    
    for( int i=0; i<index; i++ ) {
        message[i] = (char)( serverSocketBuffer.getElementDirect( 0 ) );
        serverSocketBuffer.deleteElement( 0 );
        }
    // delete message terminal character
    serverSocketBuffer.deleteElement( 0 );
    
    message[ index ] = '\0';
    
    overheadServerBytesRead += 52;
    
    return message;
    }



typedef enum messageType {
	MAP_CHUNK,
    MAP_CHANGE,
    PLAYER_UPDATE,
    PLAYER_MOVES_START,
    UNKNOWN
    } messageType;




messageType getMessageType( char *inMessage ) {
    char *copy = stringDuplicate( inMessage );
    
    char *firstBreak = strstr( copy, "\n" );
    
    if( firstBreak == NULL ) {
        delete [] copy;
        return UNKNOWN;
        }
    
    firstBreak[0] = '\0';
    
    messageType returnValue = UNKNOWN;

    if( strcmp( copy, "MC" ) == 0 ) {
        returnValue = MAP_CHUNK;
        }
    else if( strcmp( copy, "MX" ) == 0 ) {
        returnValue = MAP_CHANGE;
        }
    else if( strcmp( copy, "PU" ) == 0 ) {
        returnValue = PLAYER_UPDATE;
        }
    else if( strcmp( copy, "PM" ) == 0 ) {
        returnValue = PLAYER_MOVES_START;
        }
    
    delete [] copy;
    return returnValue;
    }




typedef struct LiveObject {
        int id;

        int holdingID;

        // current fractional grid position and speed
        doublePair currentPos;
        // current speed is move delta per frame
        doublePair currentSpeed;

        // for instant reaction to move command when server hasn't
        // responded yet
        // in grid spaces per sec
        double lastSpeed;

        // recompute speed periodically during move so that we don't
        // fall behind when frame rate fluctuates
        double timeOfLastSpeedUpdate;
        
        // destination grid position
        int xd;
        int yd;
        
        // how long whole move should take
        double moveTotalTime;
        
        // wall clock time in seconds object should arrive
        double moveEtaTime;

        
        char inMotion;
        
        char displayChar;

        char pendingAction;
        float pendingActionAnimationProgress;
        
    } LiveObject;


SimpleVector<LiveObject> gameObjects;




void updateMoveSpeed( LiveObject *inObject ) {
    doublePair endPos = { (double)inObject->xd, (double)inObject->yd };
    
    double etaSec = inObject->moveEtaTime - game_getCurrentTime();
    
    doublePair moveLeft = sub( endPos, 
                               inObject->currentPos );

    doublePair speedPerSec =
        mult( moveLeft, 1.0 / etaSec );
                            
    inObject->currentSpeed =
        mult( speedPerSec, 
              1.0 / getRecentFrameRate() );

    inObject->timeOfLastSpeedUpdate = game_getCurrentTime();
    }





int ourID;

char lastCharUsed = 'A';


void drawFrame( char inUpdate ) {    


    if( !inUpdate ) {

        if( isQuittingBlocked() ) {
            // unsafe NOT to keep updating here, because pending network
            // requests can stall

            // keep stepping current page, but don't do any other processing
            // (and still block user events from reaching current page)
            //if( currentGamePage != NULL ) {
            //    currentGamePage->base_step();
            //    }
            }

        drawFrameNoUpdate( false );
            
        drawPauseScreen();
        
        if( !wasPaused ) {
            //if( currentGamePage != NULL ) {
            //    currentGamePage->base_makeNotActive();
            //    }

            // fade out music during pause
            //setMusicLoudness( 0 );

            // unhold E key
            eKeyDown = false;
            }
        wasPaused = true;

        // handle delete key repeat
        if( holdDeleteKeySteps > -1 ) {
            holdDeleteKeySteps ++;
            
            if( holdDeleteKeySteps > stepsBetweenDeleteRepeat ) {        
                // delete repeat

                // platform layer doesn't receive event for key held down
                // tell it we are still active so that it doesn't
                // reduce the framerate during long, held deletes
                wakeUpPauseFrameRate();
                


                // subtract from messsage
                deleteCharFromUserTypedMessage();
                
                            

                // shorter delay for subsequent repeats
                stepsBetweenDeleteRepeat = (int)( 2/ frameRateFactor );
                holdDeleteKeySteps = 0;
                }
            }

        // fade in pause screen
        if( pauseScreenFade < 1 ) {
            pauseScreenFade += ( 1.0 / 30 ) * frameRateFactor;
        
            if( pauseScreenFade > 1 ) {
                pauseScreenFade = 1;
                }
            }
        

        return;
        }


    // not paused


    // fade pause screen out
    if( pauseScreenFade > 0 ) {
        pauseScreenFade -= ( 1.0 / 30 ) * frameRateFactor;
        
        if( pauseScreenFade < 0 ) {
            pauseScreenFade = 0;

            if( currentUserTypedMessage != NULL ) {

                // make sure it doesn't already end with a file separator
                // (never insert two in a row, even when player closes
                //  pause screen without typing anything)
                int lengthCurrent = strlen( currentUserTypedMessage );

                if( lengthCurrent < 2 ||
                    currentUserTypedMessage[ lengthCurrent - 2 ] != 28 ) {
                         
                        
                    // insert at file separator (ascii 28)
                    
                    char *oldMessage = currentUserTypedMessage;
                    
                    currentUserTypedMessage = autoSprintf( "%s %c ", 
                                                           oldMessage,
                                                           28 );
                    delete [] oldMessage;
                    }
                }
            }
        }    
    
    

    if( !firstDrawFrameCalled ) {
        
        // do final init step... stuff that shouldn't be done until
        // we have control of screen
        
        char *moveKeyMapping = 
            SettingsManager::getStringSetting( "upLeftDownRightKeys" );
    
        if( moveKeyMapping != NULL ) {
            char *temp = stringToLowerCase( moveKeyMapping );
            delete [] moveKeyMapping;
            moveKeyMapping = temp;
        
            if( strlen( moveKeyMapping ) == 4 &&
                strcmp( moveKeyMapping, "wasd" ) != 0 ) {
                // different assignment

                upKey = moveKeyMapping[0];
                leftKey = moveKeyMapping[1];
                downKey = moveKeyMapping[2];
                rightKey = moveKeyMapping[3];
                }
            delete [] moveKeyMapping;
            }


        serverSocket = openSocketConnection( serverAddress, serverPort );


        firstDrawFrameCalled = true;
        }

    if( wasPaused ) {
        //if( currentGamePage != NULL ) {
        //    currentGamePage->base_makeActive( false );
        //    }

        // fade music in
        //if( ! musicOff ) {
        //    setMusicLoudness( 1.0 );
        //    }
        wasPaused = false;
        }



    // updates here
    
    
    // first, read all available data from server
    readServerSocketFull();
    

    char *message = getNextServerMessage();


    if( message != NULL ) {
        printf( "Got length %d message\n%s\n", strlen( message ), message );

        messageType type = getMessageType( message );
        

        if( type == MAP_CHUNK ) {
            
            int size = 0;
            int x = 0;
            int y = 0;
            
            int binarySize = 0;
            int compressedSize = 0;
            
            sscanf( message, "MC\n%d %d %d\n%d %d\n", 
                    &size, &x, &y, &binarySize, &compressedSize );
            
            printf( "Got map chunk with bin size %d, compressed size %d\n", 
                    binarySize, compressedSize );
            
            // recenter our in-ram sub-map around this new chunk
            mapOffsetX = x + size/2;
            mapOffsetY = y + size/2;
                        
            
            unsigned char *compressedChunk = 
                new unsigned char[ compressedSize ];
    
            
            for( int i=0; i<compressedSize; i++ ) {
                compressedChunk[i] = serverSocketBuffer.getElementDirect( 0 );
                
                serverSocketBuffer.deleteElement( 0 );
                }

            /*
            unsigned char *decompressedChunk =
                zipDecompress( compressedChunk, 
                               compressedSize,
                               binarySize );
            
            delete [] compressedChunk;
            */
            
            unsigned char *binaryChunk = new unsigned char[ binarySize + 1 ];
            
            //memcpy( binaryChunk, decompressedChunk, binarySize );
            memcpy( binaryChunk, compressedChunk, binarySize );
            //delete [] decompressedChunk;
            delete [] compressedChunk;

            // for now, binary chunk is actually just ASCII
            binaryChunk[ binarySize ] = '\0';
            
            printf( "Chunk = \n%s\n", binaryChunk );

            SimpleVector<char *> *tokens = 
                tokenizeString( (char*)binaryChunk );
            
            delete [] binaryChunk;

            // first four are header parts

            int numCells = size * size;

            if( tokens->size() == numCells ) {
                
                for( int i=4; i<tokens->size(); i++ ) {
                    int cI = i-4;
                    int cX = cI % size;
                    int cY = cI / size;
                    
                    int mapX = cX + x - mapOffsetX + mapD / 2;
                    int mapY = cY + y - mapOffsetY + mapD / 2;
                    
                    if( mapX >= 0 && mapX < mapD
                        &&
                        mapY >= 0 && mapY < mapD ) {
                        
                        
                        int mapI = mapY * mapD + mapX;
                        
                        sscanf( tokens->getElementDirect(i),
                                "%d", &( map[mapI] ) );
                        }
                    }
                }   
            
            tokens->deallocateStringElements();
            delete tokens;

            firstServerMessagesReceived |= 1;
            }
        else if( type == MAP_CHANGE ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip fist
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {
                
                int x, y, id;
                int numRead = sscanf( lines[i], "%d %d %d",
                                      &x, &y, &id );
                if( numRead == 3 ) {
                    int mapX = x - mapOffsetX + mapD / 2;
                    int mapY = y - mapOffsetY + mapD / 2;
                    
                    if( mapX >= 0 && mapX < mapD
                        &&
                        mapY >= 0 && mapY < mapD ) {
                        
                        map[mapY * mapD + mapX ] = id;
                        }
                    }
                
                delete [] lines[i];
                }
            
            delete [] lines;
            }
        else if( type == PLAYER_UPDATE ) {
            
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip fist
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                LiveObject o;

                int numRead = sscanf( lines[i], "%d %d %d %d %lf",
                                      &( o.id ),
                                      &( o.holdingID ),
                                      &( o.xd ),
                                      &( o.yd ),
                                      &( o.lastSpeed ) );
                
                if( numRead == 5 ) {
                    
                    LiveObject *existing = NULL;

                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == o.id ) {
                            existing = gameObjects.getElement(j);
                            break;
                            }
                        }
                    
                    if( existing != NULL ) {
                        existing->holdingID = o.holdingID;
                        
                        
                        
                        existing->lastSpeed = o.lastSpeed;
                        
                        if( existing->id != ourID ) {
                            // don't ever force-update these for
                            // our locally-controlled object
                            // give illusion of it being totally responsive
                            // to move commands
                            existing->currentPos.x = o.xd;
                            existing->currentPos.y = o.yd;
                        
                            existing->currentSpeed.x = 0;
                            existing->currentSpeed.y = 0;
                        
                            existing->xd = o.xd;
                            existing->yd = o.yd;
                            }
                        else {
                            // update for us
                            
                            if( !existing->inMotion ) {
                                // this is an update post-action, not post-move
                                
                                // ready to execute next action
                                playerActionPending = false;
                                
                                existing->pendingAction = false;
                                }
                            }
                        
                        // in motion until update received, now done
                        existing->inMotion = false;
                        
                        existing->moveTotalTime = 0;
                        }
                    else {    
                        o.displayChar = lastCharUsed + 1;
                    
                        lastCharUsed = o.displayChar;
                    

                        o.inMotion = false;

                        o.pendingAction = false;
                        o.pendingActionAnimationProgress = 0;
                        
                        o.currentPos.x = o.xd;
                        o.currentPos.y = o.yd;
                        
                        o.currentSpeed.x = 0;
                        o.currentSpeed.y = 0;
                        
                        o.moveTotalTime = 0;
                        
                        
                        gameObjects.push_back( o );
                        }
                    }
                else if( numRead == 2 ) {
                    if( strstr( lines[i], "X X" ) != NULL  ) {
                        // object deleted
                        
                        numRead = sscanf( lines[i], "%d %d",
                                          &( o.id ),
                                          &( o.holdingID ) );
                        

                        for( int i=0; i<gameObjects.size(); i++ ) {
        
                            if( gameObjects.getElement( i )->id == o.id ) {
                                gameObjects.deleteElement( i );
                                break;
                                }
                            }
                        
                        }
                    }
                
                delete [] lines[i];
                }
            

            delete [] lines;


            if( ( firstServerMessagesReceived & 2 ) == 0 ) {
            
                LiveObject *ourObject = 
                    gameObjects.getElement( gameObjects.size() - 1 );
                
                ourID = ourObject->id;
                
                ourObject->displayChar = 'A';

                // center view on player's starting position
                lastScreenViewCenter.x = 32 * ourObject->xd;
                lastScreenViewCenter.y = 32 * ourObject->yd;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                }
            
            firstServerMessagesReceived |= 2;
            }
        else if( type == PLAYER_MOVES_START ) {
            
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip fist
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                LiveObject o;

                double etaSec;
                
                int startX, startY;
                int deltaX, deltaY;
                
                int numRead = sscanf( lines[i], "%d %d %d %d %d %lf %lf",
                                      &( o.id ),
                                      &( startX ),
                                      &( startY ),
                                      &( deltaX ),
                                      &( deltaY ),
                                      &( o.moveTotalTime ),
                                      &etaSec );
                
                o.xd = startX + deltaX;
                o.yd = startY + deltaY;
                
                o.moveEtaTime = etaSec + game_getCurrentTime();
                

                if( numRead == 7 ) {
                    
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == o.id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            double timePassed = 
                                o.moveTotalTime - etaSec;
                            
                            double fractionPassed = 
                                timePassed / o.moveTotalTime;
                            
                            doublePair startPos = { (double)startX,
                                                    (double)startY };
                            doublePair endPos = { (double)o.xd,
                                                  (double)o.yd };
                            
                            
                            
                            
                            // stays in motion until we receive final
                            // PLAYER_UPDATE from server telling us
                            // that move is over
                            existing->inMotion = true;
                            
                            if( existing->id != ourID ) {
                                // don't force-update these
                                // for our object
                                // we control it locally, to keep
                                // illusion of full move interactivity
                            
                                if( equal( existing->currentPos, startPos ) ) {
                                
                                    existing->currentPos = 
                                        add( mult( endPos, 
                                                   fractionPassed ), 
                                             mult( startPos, 
                                                   1 - fractionPassed ) );
                                    }
                                
                                
                                existing->xd = o.xd;
                                existing->yd = o.yd;
                                
                                existing->moveTotalTime = o.moveTotalTime;
                                existing->moveEtaTime = o.moveEtaTime;

                                updateMoveSpeed( existing );
                                }
                            
                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            

            delete [] lines;
            }
        

        delete [] message;
        }
    
        
    // check if we're about to move off the screen
    LiveObject *ourLiveObject = NULL;

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == ourID ) {
            ourLiveObject = o;
            break;
            }
        }

    
    if( ourLiveObject != NULL ) {
        
        doublePair screenDest = { (double)( ourLiveObject->xd * 32 ), 
                                  (double)( ourLiveObject->yd * 32 ) };
        
        doublePair dir = sub( screenDest, lastScreenViewCenter );
        
        char viewChange = false;
        
        int maxR = 150;
        double moveSpeedFactor = 0.02;

        if( fabs( dir.x ) > maxR ) {
            viewChange = true;
        
            double moveMagnitudeX = fabs( dir.x ) - maxR;
            
            moveMagnitudeX *= moveSpeedFactor / frameRateFactor;

            // bin it to whole numbered bins
            moveMagnitudeX = ceil( moveMagnitudeX );
                            
            if( dir.x < 0 ) {
                moveMagnitudeX *= -1;
                }
            

            lastScreenViewCenter.x += moveMagnitudeX;
            }
        if( fabs( dir.y ) > maxR ) {
            viewChange = true;
        
            double moveMagnitudeY = fabs( dir.y ) - maxR;
            
            moveMagnitudeY *= moveSpeedFactor / frameRateFactor;

            // bin it to whole numbered bins
            moveMagnitudeY = ceil( moveMagnitudeY );
                            
            if( dir.y < 0 ) {
                moveMagnitudeY *= -1;
                }
            

            lastScreenViewCenter.y += moveMagnitudeY;
            }
        

        if( viewChange ) {
            
            setViewCenterPosition( lastScreenViewCenter.x, 
                                   lastScreenViewCenter.y );
            
            }

        }
    
    
    // update all positions for moving objects
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->currentSpeed.x != 0 ||
            o->currentSpeed.y != 0 ) {

            doublePair endPos = { (double)o->xd, (double)o->yd };
            
            if( distance( endPos, o->currentPos )
                < length( o->currentSpeed ) ) {
                
                // reached destination
                o->currentPos = endPos;
                o->currentSpeed.x = 0;
                o->currentSpeed.y = 0;
                }
            else {
                // still stepping toward it
                o->currentPos =
                    add( o->currentPos,
                         o->currentSpeed );
                }
            
            // correct move speed based on how far we have left to go
            // and eta wall-clock time
            
            // make this correction once per second
            if( game_getCurrentTime() - o->timeOfLastSpeedUpdate
                > .25 ) {
    
                //updateMoveSpeed( o );
                }
            
            }

        if( o->id == ourID &&
            ( o->pendingAction || o->pendingActionAnimationProgress != 0 ) ) {
            
            o->pendingActionAnimationProgress += 0.05 * frameRateFactor;
            
            if( o->pendingActionAnimationProgress > 1 ) {
                if( o->pendingAction ) {
                    // still pending, wrap around smoothly
                    o->pendingActionAnimationProgress -= 1;
                    }
                else {
                    // no longer pending, finish last cycle by snapping
                    // back to 0
                    o->pendingActionAnimationProgress = 0;
                    }
                }
            }
        }
    

    if( nextActionMessageToSend != NULL 
        && ourLiveObject->currentSpeed.x == 0
        && ourLiveObject->currentSpeed.y == 0 ) {
        
        // done moving on client end
        // can start showing pending action animation, even if 
        // end of motion not received from server yet

        if( !playerActionPending ) {
            playerActionPending = true;
            ourLiveObject->pendingAction = true;
            
            // start on first frame to force at least one cycle no
            // matter how fast the server responds
            ourLiveObject->pendingActionAnimationProgress = 
                0.05 * frameRateFactor;
            }
        
        
        if( ! ourLiveObject->inMotion && 
            ourLiveObject->pendingActionAnimationProgress > 0.25 ) {
            
            // move end acked by server AND action animation in progress

            // queued action waiting for our move to end
            sendToSocket( serverSocket, 
                          (unsigned char*)nextActionMessageToSend, 
                          strlen( nextActionMessageToSend) );
            
            numServerBytesSent += strlen( nextActionMessageToSend );
            overheadServerBytesSent += 52;

            delete [] nextActionMessageToSend;
            nextActionMessageToSend = NULL;
            }
        }
    


    // now draw stuff AFTER all updates
    drawFrameNoUpdate( true );



    // draw tail end of pause screen, if it is still visible
    if( pauseScreenFade > 0 ) {
        drawPauseScreen();
        }
    }



void drawFrameNoUpdate( char inUpdate ) {
    
    setDrawColor( 1, 1, 1, 1 );
    drawSquare( lastScreenViewCenter, 400 );
    
    //if( currentGamePage != NULL ) {
    //    currentGamePage->base_draw( lastScreenViewCenter, viewWidth );
    //    }
    
    setDrawColor( 1, 1, 1, 1 );

    int gridCenterX = 
        lrintf( lastScreenViewCenter.x / 32 ) - mapOffsetX + mapD/2;
    int gridCenterY = 
        lrintf( lastScreenViewCenter.y / 32 ) - mapOffsetY + mapD/2;
    
    int xStart = gridCenterX - 12;
    int xEnd = gridCenterX + 12;

    int yStart = gridCenterY - 12;
    int yEnd = gridCenterY + 12;

    if( xStart < 0 ) {
        xStart = 0;
        }
    if( xStart >= mapD ) {
        xStart = mapD - 1;
        }
    
    if( yStart < 0 ) {
        yStart = 0;
        }
    if( yStart >= mapD ) {
        yStart = mapD - 1;
        }

    if( xEnd < 0 ) {
        xEnd = 0;
        }
    if( xEnd >= mapD ) {
        xEnd = mapD - 1;
        }
    
    if( yStart < 0 ) {
        yStart = 0;
        }
    if( yEnd >= mapD ) {
        yEnd = mapD - 1;
        }
    

    for( int y=yEnd; y>=yStart; y-- ) {
        
        int screenY = 32 * ( y + mapOffsetY - mapD / 2 );
        
        for( int x=xStart; x<=xEnd; x++ ) {
            
            int screenX = 32 * ( x + mapOffsetX - mapD / 2 );

            int mapI = y * mapD + x;
            
            int oID = map[ mapI ];
            
            if( oID > 0 ) {
                
                doublePair pos = { (double)screenX, (double)screenY };

                drawObject( getObject(oID), pos );
                }
            }
        }
        
    
    
    
    
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );


        if( o->currentPos.x != o->xd || o->currentPos.y != o->yd ) {
            // destination
            
            char *string = autoSprintf( "[%c]", o->displayChar );
        
            doublePair pos;
            pos.x = o->xd * 32;
            pos.y = o->yd * 32;
        
            setDrawColor( 1, 0, 0, 1 );
            mainFont->drawString( string, 
                                  pos, alignCenter );
            delete [] string;
            }



        // current pos
        char *string = autoSprintf( "%c", o->displayChar );

        doublePair pos = mult( o->currentPos, 32 );

        doublePair actionOffset = { 0, 0 };
        
        
        if( o->id == ourID && o->pendingActionAnimationProgress != 0 ) {
            // wiggle toward target

            float xDir = 0;
            float yDir = 0;
            
            if( o->xd < playerActionTargetX ) {
                xDir = 1;
                }
            if( o->xd > playerActionTargetX ) {
                xDir = -1;
                }
            if( o->yd < playerActionTargetY ) {
                yDir = 1;
                }
            if( o->yd > playerActionTargetY ) {
                yDir = -1;
                }
            
            double offset =
                8 - 
                8 * cos( 2 * M_PI * o->pendingActionAnimationProgress );
            

            actionOffset.x += xDir * offset;
            actionOffset.y += yDir * offset;
            }
        

        // bare hands action OR holding something
        // character wiggle
        if(  o->id == ourID && o->pendingActionAnimationProgress != 0 ) {
            
            pos = add( pos, actionOffset );
            }
        

        setDrawColor( 0, 0, 0, 1 );
        mainFont->drawString( string, 
                              pos, alignCenter );

        delete [] string;
        
        if( o->holdingID != 0 ) { 
            doublePair holdPos = pos;
            holdPos.x += 16;
            holdPos.y -= 16;
            
            setDrawColor( 1, 1, 1, 1 );
            drawObject( getObject( o->holdingID ), holdPos );
            }
        
        
        }


    setDrawColor( 0, 0, 0, 0.125 );
    
    int screenGridOffsetX = lrint( lastScreenViewCenter.x / 32 );
    int screenGridOffsetY = lrint( lastScreenViewCenter.y / 32 );
    
    for( int y=-11; y<12; y++ ) {
        for( int x=-11; x<12; x++ ) {
            
            doublePair pos;
            pos.x = ( x + screenGridOffsetX ) * 32;
            pos.y = ( y + screenGridOffsetY ) * 32;
            
            drawSquare( pos, 14 );
            }
        }
    
    }



// store mouse data for use as unguessable randomizing data
// for key generation, etc.
#define MOUSE_DATA_BUFFER_SIZE 20
int mouseDataBufferSize = MOUSE_DATA_BUFFER_SIZE;
int nextMouseDataIndex = 0;
// ensure that stationary mouse data (same value over and over)
// doesn't overwrite data from actual motion
float lastBufferedMouseValue = 0;
float mouseDataBuffer[ MOUSE_DATA_BUFFER_SIZE ];



void pointerMove( float inX, float inY ) {

    // save all mouse movement data for key generation
    float bufferValue = inX + inY;
    // ignore mouse positions that are the same as the last one
    // only save data when mouse actually moving
    if( bufferValue != lastBufferedMouseValue ) {
        
        mouseDataBuffer[ nextMouseDataIndex ] = bufferValue;
        lastBufferedMouseValue = bufferValue;
        
        nextMouseDataIndex ++;
        if( nextMouseDataIndex >= mouseDataBufferSize ) {
            nextMouseDataIndex = 0;
            }
        }
    

    if( isPaused() ) {
        return;
        }
    
    }



static char isGridAdjacent( int inXA, int inYA, int inXB, int inYB ) {
    if( ( abs( inXA - inXB ) == 1 && inYA == inYB ) 
        ||
        ( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {
        
        return true;
        }

    return false;
    }


void pointerDown( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }

    if( firstServerMessagesReceived != 3 ) {
        return;
        }

    if( playerActionPending ) {
        // block further actions until update received to confirm last
        // action
        return;
        }
    

    LiveObject *ourLiveObject;

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == ourID ) {
            ourLiveObject = o;
            break;
            }
        }
    
    int clickDestX = lrintf( ( inX ) / 32 );
    
    int clickDestY = lrintf( ( inY ) / 32 );
    

    if( clickDestX == ourLiveObject->xd && clickDestY == ourLiveObject->yd ) {
        // ignore clicks where we're already standing
        return;
        }
    
    
    // may change to empty adjacent spot to click
    int moveDestX = clickDestX;
    int moveDestY = clickDestY;
    
    char mustMove = false;


    int destID = 0;
        
    int mapX = clickDestX - mapOffsetX + mapD / 2;
    int mapY = clickDestY - mapOffsetY + mapD / 2;
    
    printf( "clickDestX,Y = %d, %d,  mapX,Y = %d, %d, curX,Y = %d, %d\n", 
            clickDestX, clickDestY, 
            mapX, mapY,
            ourLiveObject->xd, ourLiveObject->yd );
    if( mapY >= 0 && mapY < mapD &&
        mapX >= 0 && mapX < mapD ) {
        
        destID = map[ mapY * mapD + mapX ];
        }
    
    printf( "DestID = %d\n", destID );
        

    if( nextActionMessageToSend != NULL ) {
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }
    

    char modClick = false;
    
    if( eKeyDown || isLastMouseButtonRight() ) {
        modClick = true;
        }

    if( destID == 0 && ( !modClick || ourLiveObject->holdingID == 0 ) ) {
        // a move to an empty spot
        // can interrupt current move
        
        mustMove = true;
        }
    else if( modClick || destID != 0 ) {
        // use/drop modifier
        // OR pick up action
            
        
        char canExecute = false;
        
        // direct click on adjacent cells?
        if( isGridAdjacent( clickDestX, clickDestY,
                            ourLiveObject->xd, ourLiveObject->yd ) ) {
            
            canExecute = true;
            }
        else {
            // need to move to empty adjacent first, if it exists
            
            
            int nDX[4] = { -1, +1, 0, 0 };
            int nDY[4] = { 0, 0, -1, +1 };
            
            char foundEmpty = false;
            
            double closestDist = 9999999;
            

            for( int n=0; n<4; n++ ) {
                int x = mapX + nDX[n];
                int y = mapY + nDY[n];

                if( y >= 0 && y < mapD &&
                    x >= 0 && x < mapD ) {
                 
                    
                    if( map[ y * mapD + x ] == 0 ) {
                        
                        int emptyX = clickDestX + nDX[n];
                        int emptyY = clickDestY + nDY[n];

                        doublePair emptyDest = { (double)emptyX, 
                                                 (double)emptyY };
                        
                        double emptyDist = 
                            distance( emptyDest, ourLiveObject->currentPos );
                        
                        if( emptyDist < closestDist ) {
                            
                            moveDestX = emptyX;
                            moveDestY = emptyY;
                            
                            closestDist = emptyDist;
                        
                            foundEmpty = true;
                            }
                        }
                    
                    }
                }
            
            if( foundEmpty ) {
                canExecute = true;
                mustMove = true;
                }
            }
        
        if( canExecute ) {
            
            const char *action = "";
            
            char send = false;
            
                
            if( modClick && destID == 0 && ourLiveObject->holdingID != 0 ) {
                action = "DROP";
                send = true;
                }
            else if( modClick && destID != 0 ) {
                action = "USE";
                send = true;
                }
            else if( ! modClick && destID != 0 ) {
                action = "GRAB";
                send = true;
                }
            
            
            if( send ) {
                // queue this until after we are done moving, if we are
                nextActionMessageToSend = 
                    autoSprintf( "%s %d %d#", action,
                                 clickDestX, clickDestY );
                playerActionTargetX = clickDestX;
                playerActionTargetY = clickDestY;
                }
            }
        }
    


    
    if( mustMove ) {
        

        // send move right away
        char *message = autoSprintf( "MOVE %d %d#", moveDestX, moveDestY );
        sendToSocket( serverSocket, (unsigned char*)message, 
                      strlen( message ) );
            
        numServerBytesSent += strlen( message );
        overheadServerBytesSent += 52;

        delete [] message;

        // start moving before we hear back from server

        ourLiveObject->xd = moveDestX;
        ourLiveObject->yd = moveDestY;
        ourLiveObject->inMotion = true;

        doublePair endPos = { (double)moveDestX, (double)moveDestY };
            
        ourLiveObject->moveTotalTime = 
            distance( endPos, 
                      ourLiveObject->currentPos ) / 
            ourLiveObject->lastSpeed;

        ourLiveObject->moveEtaTime = game_getCurrentTime() +
            ourLiveObject->moveTotalTime;

            
        updateMoveSpeed( ourLiveObject );
        }    
                
    
    }



void pointerDrag( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }
    }



void pointerUp( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }
    }







void keyDown( unsigned char inASCII ) {

    // taking screen shot is ALWAYS possible
    if( inASCII == '=' ) {    
        saveScreenShot( "screen" );
        }
    

    
    if( isPaused() ) {
        // block general keyboard control during pause


        switch( inASCII ) {
            case 13:  // enter
                // unpause
                pauseGame();
                break;
            }
        
        
        if( inASCII == 127 || inASCII == 8 ) {
            // subtract from it

            deleteCharFromUserTypedMessage();

            holdDeleteKeySteps = 0;
            // start with long delay until first repeat
            stepsBetweenDeleteRepeat = (int)( 30 / frameRateFactor );
            }
        else if( inASCII >= 32 ) {
            // add to it
            if( currentUserTypedMessage != NULL ) {
                
                char *oldMessage = currentUserTypedMessage;

                currentUserTypedMessage = autoSprintf( "%s%c", 
                                                       oldMessage, inASCII );
                delete [] oldMessage;
                }
            else {
                currentUserTypedMessage = autoSprintf( "%c", inASCII );
                }
            }
        
        return;
        }
    

    
    switch( inASCII ) {
        case 'e':
        case 'E':
            eKeyDown = true;
            break;
        case 'm':
        case 'M': {
#ifdef USE_MALLINFO
            struct mallinfo meminfo = mallinfo();
            printf( "Mem alloc: %d\n",
                    meminfo.uordblks / 1024 );
#endif
            }
            break;
        }
    }



void keyUp( unsigned char inASCII ) {
    if( inASCII == 127 || inASCII == 8 ) {
        // delete no longer held
        // even if pause screen no longer up, pay attention to this
        holdDeleteKeySteps = -1;
        }

    if( ! isPaused() ) {
        
        switch( inASCII ) {
            case 'e':
            case 'E':
                eKeyDown = false;
                break;
            }
        }

    }







void specialKeyDown( int inKey ) {
    if( isPaused() ) {
        return;
        }
    
	}



void specialKeyUp( int inKey ) {
    if( isPaused() ) {
        return;
        }
    
	} 




char getUsesSound() {
    
    return ! musicOff;
    }









void drawString( const char *inString, char inForceCenter ) {
    
    setDrawColor( 1, 1, 1, 0.75 );

    doublePair messagePos = lastScreenViewCenter;

    TextAlignment align = alignCenter;
    
    if( initDone && !inForceCenter ) {
        // transparent message
        setDrawColor( 1, 1, 1, 0.75 );

        // stick messages in corner
        messagePos.x -= viewWidth / 2;
        
        messagePos.x +=  20;
    

    
        messagePos.y += (viewWidth * viewHeightFraction) /  2;
    
        messagePos.y -= 32;

        align = alignLeft;
        }
    else {
        // fully opaque message
        setDrawColor( 1, 1, 1, 1 );

        // leave centered
        }
    

    int numLines;
    
    char **lines = split( inString, "\n", &numLines );
    
    for( int i=0; i<numLines; i++ ) {
        

        mainFont->drawString( lines[i], messagePos, align );
        messagePos.y -= 32;
        
        delete [] lines[i];
        }
    delete [] lines;
    }





// called by platform to get more samples
void getSoundSamples( Uint8 *inBuffer, int inLengthToFillInBytes ) {
    // for now, do nothing (no sound)
    }




