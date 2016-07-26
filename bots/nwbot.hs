-- Newton Wars Bot Framework
-- for Haskell
-- @author cyriax 
--
-- You might want to implement all the nice functional code just below the imports.
-- Nearly everything you might want to do can be done with these functions.
-- If you want to refactor all of the monadic stuff - feel free...
-- I had more fun with the "intelligence" (although it's stripped out here for your enjoyment).

-- For your debugging experience use:
-- trace ("some var: " ++ (show var))

-- If you have problems compiling check those packages are included:
-- (you may want to use the :main function of ghci)
--
-- old-locale array deepseq bytestring containers binary transformers mmorph
-- transformers-base mtl base-unicode-symbols monad-control lifted-base time text
-- parsec unix network resourcet random ieee754-parser
--
-- I'm not sure what's inside a normal install. But this might help (a little?).

import Control.Monad.IO.Class (MonadIO, liftIO)
import Control.Monad.Trans.Resource
import Network (connectTo, PortID (..))
import System.Environment (getArgs, getProgName)
import System.IO
import Debug.Trace
import Data.Binary.Get
import Data.Word
import qualified Data.ByteString.Lazy as BSL
import qualified Data.ByteString as BS
import System.Timeout
import Control.Monad
import Data.Maybe
import Data.Binary.IEEE754
import System.Random

---------------------------------------------------
-- All the intelligence can be implemented below --
---------------------------------------------------

-- TODO: changeme
botname :: String
botname = "Haskell Bot"

-- TODO: changeme
-- Worldmodel is all your persitent data. Feel free to change stuff.
data WorldModel = WorldModel {
    myID :: Integer,
    fireAngle :: Float,
    targets :: [(Integer, (Float,Float))]
    }

-- TODO: changeme
-- The empty worldmodel 
worldModelInit :: WorldModel
worldModelInit = WorldModel 0 0 []

-- TODO: changeme
-- Is called every time an shot was fired
shotFired :: Integer -> WorldModel -> [(Float, Float)] -> WorldModel
shotFired player worldmodel shot
    | player == myID worldmodel = worldmodel {fireAngle = fireAngle worldmodel + 121.05}
    | otherwise = worldmodel

-- TODO: changeme
-- Is called every time new position information was recieved
newPlayerPosition :: Integer -> WorldModel -> (Float, Float) -> WorldModel
newPlayerPosition player worldmodel (posx,posy) =
    worldmodel {
        targets = (player,(posx,posy)):
        [(pl,pos)|(pl,pos) <- (targets worldmodel), pl /= player]}


-- TODO: changeme
-- Is called every time a player left (duh!)
playerLeft :: Integer -> WorldModel -> WorldModel
playerLeft player worldmodel = worldmodel


-- TODO: changeme
-- Sends the fire command
-- Called for every recieved package (Just in case...)
fireShot :: WorldModel -> [Char]
fireShot worldmodel = show $ fireAngle worldmodel

-- TODO: changeme
-- Is called one my id is send (should happen only once)
myIdUpdate :: Integer -> WorldModel -> WorldModel
myIdUpdate playerID worldmodel = worldmodel { myID = playerID }

---------------------------------------------
---------------------------------------------
-- stuff you might not want to touch below --
---------------------------------------------
---------------------------------------------

-- it's not even functional anymore.
-- don't do do!
-- trust me, it's not elegant.
-- really!
-- just turn away now!

----------------------------------
-- Main and Connection Handling --
----------------------------------

-- Connect Bot or just print useless^h^h^h^hage info
main :: IO ()
main = do
  args <- getArgs
  case args of
      [host, port] -> botConnect host (read port :: Int)
      _ -> usage
  where
  usage = do
      name <- getProgName
      putStrLn $ "Usage : " ++ name ++ " host port"

-- Establish actual telnet connection
botConnect :: String -> Int -> IO ()
botConnect host port = runResourceT $ do
    (releaseSock, hsock) <- allocate (connectTo host $ PortNumber $ fromIntegral port) hClose
    liftIO $ botMain hsock hsock

-- Main loop and bot init
botMain :: Handle -> Handle -> IO ()
botMain hin hout = do
    hSetBuffering hin LineBuffering
    hSetBuffering hout LineBuffering

    putStrLn $ "==== " ++ botname ++ " ===="
    hPutStrLn hout $ "n " ++ botname
    flushInput hin
    hPutStrLn hout "b"
    putStrLn "Botted myself."

    botloop worldModelInit
    where
    botloop worldmodel = do
        --line <- BSL.hGet hin 4
        lineraw <- timeout 3000000 $ BS.hGet hin 8
        when (isNothing lineraw) (botloop worldmodel)
        putStrLn ""
        let (packageType,playerID) = runGet getHeader ( BSL.fromStrict (fromMaybe BS.empty lineraw))
        worldmodel <- botAction packageType playerID hin worldmodel
        --when ((fromIntegral packageType,fromIntegral playerID) == (4, myID worldmodel))
        (hPutStrLn hout (fireShot worldmodel))
        botloop worldmodel

----------------------------
-- Package Interpretation --
----------------------------

-- Select action by package type
botAction :: Word32 -> Word32 -> Handle -> WorldModel -> IO WorldModel
botAction typeID pID hin worldmodel
    | typeID == 1 = actionMyId pID worldmodel
    | typeID == 2 = actionPlayerLeft pID worldmodel
    | typeID == 3 = actionPosition pID hin worldmodel
    | typeID == 4 = actionShot pID hin worldmodel
    | otherwise = do
        putStrLn $ "[WARN]: Unknown package type: " ++ show typeID
        return worldmodel

-- Handle my id event
actionMyId :: Word32 -> WorldModel -> IO WorldModel
actionMyId id wm = do
    putStrLn $ "I am id " ++ show id
    return $ myIdUpdate (fromIntegral id) wm

-- Handle player left event
actionPlayerLeft :: Word32 -> WorldModel -> IO WorldModel
actionPlayerLeft id wm = do
    putStrLn $ "Player with id " ++ (show id) ++ " left"
    return $ playerLeft (fromIntegral id) wm

-- Handle player position event
actionPosition :: Word32 -> Handle -> WorldModel -> IO WorldModel
actionPosition id hin wm = do
    raw <- BS.hGet hin 8
    let unpacked = BSL.unpack $BSL.fromStrict raw
    let x = parseFloatLE $ Prelude.take 4 unpacked
    let y = parseFloatLE $ Prelude.drop 4 unpacked
    putStrLn $ "Player with id " ++ (show id) ++ " has position " ++ (show x) ++ " / " ++ (show y)
    return $ newPlayerPosition (fromIntegral id) wm (x,y)

-- Handle shot event
actionShot :: Word32 -> Handle -> WorldModel -> IO WorldModel
actionShot id hin wm = do
    raw <- BS.hGet hin 4
    let shotsize = runGet getWord32le $ BSL.fromStrict raw 
    putStrLn $ "Player with id " ++ (show id) ++ " fired a shot with length " ++ show shotsize
    rawshot <- BS.hGet hin $ 8 * fromIntegral shotsize
    let shot = parseShot $ BSL.unpack $ BSL.fromStrict rawshot 
    --putStrLn $ "shot " ++ (show shot)
    return $ shotFired (fromIntegral id) wm shot

----------------------
-- Raw Data Parsing --
----------------------

-- Unpack raw header data
getHeader :: Get(Word32, Word32)
getHeader = do
    typ <- getWord32le
    pid <- getWord32le
    return (typ, pid)

-- Unpack shot data
parseShot :: [Word8] -> [(Float,Float)]
parseShot (a1:a2:a3:a4:a5:a6:a7:a8:r) = (parseFloatLE (a1:a2:a3:a4:[]), parseFloatLE (a5:a6:a7:a8:[])):parseShot r
parseShot [] = []
parseShot _ = trace("Shot parse Failed") []

-- Ignore initial input
flushInput :: Handle -> IO ()
flushInput hin = do
    r <- timeout 1000000 $ BS.hGet hin 4
    case r of
        Nothing -> putStrLn "Done Flushing."
        _ -> flushInput hin
