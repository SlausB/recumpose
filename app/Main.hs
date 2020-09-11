{-# LANGUAGE OverloadedStrings #-}

module Main where

import Lib

import Control.Applicative((<*))
import Text.Parsec
import Text.Parsec.String
import Text.Parsec.Expr
import Text.Parsec.Token
import Text.Parsec.Language
import qualified Data.Graph as G
import Data.Graph.Visualize
import Data.Graph.Types (Labeled)

-- graphviz:
import           Data.Graph.Inductive
import           Data.GraphViz
import           Data.GraphViz.Attributes.Complete
import           Data.GraphViz.Types.Generalised   as G
import           Data.GraphViz.Types.Monadic
import           Data.Text.Lazy                    as L
import           Data.Word
{- 
import Data.GraphViz.Types.Graph (DotGraph)
import Data.GraphViz
-}

data Expr = Var String | Con Bool | Uno Unop Expr | Duo Duop Expr Expr
    deriving Show
data Unop = Not
    deriving Show
data Duop = And | Iff
    deriving Show
data Stmt = Nop | String := Expr | If Expr Stmt Stmt | While Expr Stmt | Seq [Stmt]
    deriving Show

def = emptyDef{ commentStart = "{-"
              , commentEnd = "-}"
              , identLetter = alphaNum
              , opStart = oneOf "~&=:"
              , opLetter = oneOf "~&=:"
              , reservedOpNames = ["~", "&", "=", ":="]
              , reservedNames = ["true", "false", "nop",
                                 "if", "then", "else", "fi",
                                 "while", "do", "od"]
              }

TokenParser{ parens = m_parens
           , identifier = m_identifier
           , reservedOp = m_reservedOp
           , reserved = m_reserved
           , semiSep1 = m_semiSep1
           , whiteSpace = m_whiteSpace } = makeTokenParser def

exprparser :: Parser Expr
exprparser = buildExpressionParser table term <?> "expression"
table = [ [Prefix (m_reservedOp "~" >> return (Uno Not))]
        , [Infix (m_reservedOp "&" >> return (Duo And)) AssocLeft]
        , [Infix (m_reservedOp "=" >> return (Duo Iff)) AssocLeft]
        ]
term = m_parens exprparser
       <|> fmap Var m_identifier
       <|> (m_reserved "true" >> return (Con True))
       <|> (m_reserved "false" >> return (Con False))

mainparser :: Parser Stmt
mainparser = m_whiteSpace >> stmtparser <* eof
    where
      stmtparser :: Parser Stmt
      stmtparser = fmap Seq (m_semiSep1 stmt1)
      stmt1 = (m_reserved "nop" >> return Nop)
              <|> do { v <- m_identifier
                     ; m_reservedOp ":="
                     ; e <- exprparser
                     ; return (v := e)
                     }
              <|> do { m_reserved "if"
                     ; b <- exprparser
                     ; m_reserved "then"
                     ; p <- stmtparser
                     ; m_reserved "else"
                     ; q <- stmtparser
                     ; m_reserved "fi"
                     ; return (If b p q)
                     }
              <|> do { m_reserved "while"
                     ; b <- exprparser
                     ; m_reserved "do"
                     ; p <- stmtparser
                     ; m_reserved "od"
                     ; return (While b p)
                     }

play :: String -> IO ()
play inp = case Text.Parsec.parse mainparser "" inp of
             { Left err -> print err
             ; Right ans -> print ans
             }

source :: String
source = "if 5 then 6 else 7\
\What do you expect?"

-- main :: IO ()
-- main = play source



newtype MyNodeName = MyNodeName { name :: String } deriving( Show, Ord, Eq )
instance Labellable MyNodeName where
    toLabelValue = toLabelValue . show

graphData = [
    ( MyNodeName "node4", 4, [8]   ),     -- the first component can be of any type
    ( MyNodeName "node8", 8, []    ),
    ( MyNodeName "node7", 7, [4]   ),
    ( MyNodeName "node5", 5, [1,7] ),
    ( MyNodeName "node1", 1, [4]   )
  ]
nodeToIndex (_,a,_) = a
nodeToEdges (_,_,a) = a

{-
    5 --> 7
    |     |
    v     V
    1 --> 4 --> 8
-}
( myGraph, vertexToNode, keyToVertex ) = G.graphFromEdges graphData

sorted = Prelude.map vertexToNode $ G.topSort myGraph

ex1 :: Gr Text Text
ex1 = mkGraph [ (1,"one")
              , (3,"three")
              ]
              [ (1,3,"edge label") ]

showMyNode ( name, _, _ ) = name

myFoldNode vertex folding = folding ++ show ( showMyNode vertex )

foldGraph = Prelude.foldr myFoldNode "" myGraph

main :: IO ()
main = do
    -- plot_result <- Data.Graph.Visualize.plotUGraphPng ( Data.Graph.Visualize.DGraph myGraph ) "sorted.png"
    -- preview myGraph
    plot_result <- preview ex1
    print plot_result
    writeFile "hello.txt" "hello"

    -- print foldMap myGraph () ""

    -- print ( "Graph plotting results: " ++ plot_result )
    print sorted