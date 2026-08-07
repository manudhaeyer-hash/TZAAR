import java.util.*;

class Player {
    static final int WALL = -1;
    static final int EMPTY = 0;
    static final int[] DIRS = {16, 1, -15, -16, -1, 15};
    
    static int myId;
    static int currentPhase;
    static long startTime;
    static final long TIME_LIMIT = 40; // 40ms to be safe
    static boolean timeout = false;
    
    static int[] board = new int[256];
    static int[][] typeCounts = new int[2][3];
    
    static int makePiece(int owner, int type, int height) {
        return (1 << 11) | (owner << 10) | (type << 8) | height;
    }
    static int getOwner(int p) { return (p >> 10) & 1; }
    static int getType(int p) { return (p >> 8) & 3; }
    static int getHeight(int p) { return p & 0xFF; }

    static void initBoard() {
        Arrays.fill(board, WALL);
        for (int q = 0; q <= 8; q++) {
            for (int r = 0; r <= 8; r++) {
                int s = - (q-4) - (r-4);
                if (s >= -4 && s <= 4) {
                    if (!(q==4 && r==4)) { // Center is missing
                        board[(q+2) * 16 + (r+2)] = EMPTY;
                    }
                }
            }
        }
    }
    
    public static void main(String[] args) {
        Scanner in = new Scanner(System.in);
        myId = in.nextInt();
        initBoard();
        
        while (true) {
            currentPhase = in.nextInt();
            int pieceCount = in.nextInt();
            
            // Reset state
            for(int i=0; i<256; i++) if(board[i] > 0) board[i] = EMPTY;
            for(int i=0; i<2; i++) for(int j=0; j<3; j++) typeCounts[i][j] = 0;
            
            for (int i = 0; i < pieceCount; i++) {
                int q = in.nextInt();
                int r = in.nextInt();
                int owner = in.nextInt();
                int type = in.nextInt();
                int height = in.nextInt();
                
                int idx = (q+2) * 16 + (r+2);
                board[idx] = makePiece(owner, type, height);
                typeCounts[owner][type]++;
            }
            
            startTime = System.currentTimeMillis();
            timeout = false;
            
            String bestMoveStr = "WAIT";
            if (currentPhase == 1 || (currentPhase == 2 && canMakeMove(myId))) {
                bestMoveStr = findBestMove();
            }
            
            System.out.println(bestMoveStr);
        }
    }
    
    static boolean canMakeMove(int player) {
        int[] moves = new int[2048];
        return generateMoves(moves, player, currentPhase) > 0;
    }
    
    static int generateMoves(int[] moves, int playerToMove, int phase) {
        int moveCount = 0;
        
        // Add WAIT move in phase 2 if valid
        if (phase == 2) {
            moves[moveCount++] = 0; // 0 means WAIT
        }
        
        for (int i = 0; i < 256; i++) {
            int p = board[i];
            if (p > 0 && getOwner(p) == playerToMove) {
                int height = getHeight(p);
                for (int d : DIRS) {
                    int curr = i + d;
                    while (board[curr] == EMPTY) {
                        curr += d;
                    }
                    int target = board[curr];
                    if (target > 0) {
                        int targetOwner = getOwner(target);
                        int targetHeight = getHeight(target);
                        
                        if (phase == 1) {
                            // Must capture
                            if (targetOwner != playerToMove && height >= targetHeight) {
                                moves[moveCount++] = (i << 8) | curr;
                            }
                        } else {
                            // Phase 2: Capture or Reinforce
                            if (targetOwner == playerToMove) {
                                // Reinforce
                                moves[moveCount++] = (i << 8) | curr;
                            } else {
                                // Capture
                                if (height >= targetHeight) {
                                    moves[moveCount++] = (i << 8) | curr;
                                }
                            }
                        }
                    }
                }
            }
        }
        return moveCount;
    }
    
    static long makeMove(int move) {
        if (move == 0) return 0; // WAIT
        
        int from = (move >> 8) & 0xFF;
        int to = move & 0xFF;
        int origin = board[from];
        int target = board[to];
        
        board[from] = EMPTY;
        
        int targetOwner = getOwner(target);
        int targetType = getType(target);
        typeCounts[targetOwner][targetType]--;
        
        if (targetOwner == getOwner(origin)) {
            // Reinforce
            int newHeight = getHeight(origin) + getHeight(target);
            board[to] = makePiece(getOwner(origin), getType(origin), newHeight);
        } else {
            // Capture
            board[to] = origin;
        }
        
        return ((long)origin << 32) | ((long)target << 16) | (from << 8) | to;
    }
    
    static void unmakeMove(long undoInfo) {
        if (undoInfo == 0) return; // WAIT
        
        int from = (int)((undoInfo >> 8) & 0xFF);
        int to = (int)(undoInfo & 0xFF);
        int target = (int)((undoInfo >> 16) & 0xFFFF);
        int origin = (int)((undoInfo >> 32) & 0xFFFF);
        
        board[from] = origin;
        board[to] = target;
        
        int targetOwner = getOwner(target);
        int targetType = getType(target);
        typeCounts[targetOwner][targetType]++;
    }
    
    static String findBestMove() {
        int bestMove = 0;
        int[] moves = new int[2048];
        
        for (int depth = 1; depth <= 10; depth++) {
            int currentBest = -1;
            int maxScore = -2000000;
            
            int moveCount = generateMoves(moves, myId, currentPhase);
            if (moveCount == 0) break;
            
            for (int i = 0; i < moveCount; i++) {
                int move = moves[i];
                long undo = makeMove(move);
                
                // Next turn is opponent's Phase 1, or my Phase 2
                int nextPlayer = (currentPhase == 1) ? myId : (1 - myId);
                int nextPhase = (currentPhase == 1) ? 2 : 1;
                
                int score = alphaBeta(depth - 1, -2000000, 2000000, nextPlayer, nextPhase);
                
                unmakeMove(undo);
                
                if (timeout) break;
                
                if (score > maxScore) {
                    maxScore = score;
                    currentBest = move;
                }
            }
            
            if (timeout) break;
            if (currentBest != -1) bestMove = currentBest;
            
            // If we found a forced win, stop searching
            if (maxScore > 1000000) break;
        }
        
        if (bestMove == 0) return "WAIT";
        int from = (bestMove >> 8) & 0xFF;
        int to = bestMove & 0xFF;
        return String.format("MOVE %d %d %d %d", (from/16)-2, (from%16)-2, (to/16)-2, (to%16)-2);
    }
    
    static int alphaBeta(int depth, int alpha, int beta, int playerToMove, int phase) {
        if (System.currentTimeMillis() - startTime > TIME_LIMIT) {
            timeout = true;
            return 0;
        }
        
        // Win/Loss check
        boolean myLoss = false;
        boolean oppLoss = false;
        for (int t = 0; t < 3; t++) {
            if (typeCounts[myId][t] == 0) myLoss = true;
            if (typeCounts[1 - myId][t] == 0) oppLoss = true;
        }
        if (myLoss && !oppLoss) return -1000000 - depth;
        if (oppLoss && !myLoss) return 1000000 + depth;
        if (myLoss && oppLoss) return 0;
        
        if (depth == 0) {
            return evaluate();
        }
        
        int[] moves = new int[2048];
        int moveCount = generateMoves(moves, playerToMove, phase);
        
        if (moveCount == 0) {
            // Player has no moves. 
            // In Phase 1, this means they can't capture, so they lose!
            if (phase == 1) {
                if (playerToMove == myId) return -1000000 - depth;
                else return 1000000 + depth;
            } else {
                // In Phase 2, wait is always generated, so moveCount > 0 always.
            }
        }
        
        int nextPlayer = (phase == 1) ? playerToMove : (1 - playerToMove);
        int nextPhase = (phase == 1) ? 2 : 1;
        
        if (playerToMove == myId) {
            int maxScore = -2000000;
            for (int i = 0; i < moveCount; i++) {
                int move = moves[i];
                long undo = makeMove(move);
                int score = alphaBeta(depth - 1, alpha, beta, nextPlayer, nextPhase);
                unmakeMove(undo);
                if (timeout) return 0;
                
                if (score > maxScore) maxScore = score;
                if (maxScore > alpha) alpha = maxScore;
                if (alpha >= beta) break;
            }
            return maxScore;
        } else {
            int minScore = 2000000;
            for (int i = 0; i < moveCount; i++) {
                int move = moves[i];
                long undo = makeMove(move);
                int score = alphaBeta(depth - 1, alpha, beta, nextPlayer, nextPhase);
                unmakeMove(undo);
                if (timeout) return 0;
                
                if (score < minScore) minScore = score;
                if (minScore < beta) beta = minScore;
                if (alpha >= beta) break;
            }
            return minScore;
        }
    }
    
    static int evaluate() {
        int score = 0;
        int oppId = 1 - myId;
        
        int[] typeWeights = {300, 200, 100}; // TZAAR, TZARRA, TOTT
        
        for (int t = 0; t < 3; t++) {
            int myCount = typeCounts[myId][t];
            int oppCount = typeCounts[oppId][t];
            
            score += (myCount - oppCount) * typeWeights[t];
            
            // Vulnerability penalty
            if (myCount == 1) score -= 5000;
            if (oppCount == 1) score += 5000;
        }
        
        // Height evaluation: we want a few tall stacks, but not too tall
        // Let's just find the max height for each player
        int myMaxHeight = 0;
        int oppMaxHeight = 0;
        
        for (int i = 0; i < 256; i++) {
            int p = board[i];
            if (p > 0) {
                int owner = getOwner(p);
                int height = getHeight(p);
                if (owner == myId) {
                    if (height > myMaxHeight) myMaxHeight = height;
                } else {
                    if (height > oppMaxHeight) oppMaxHeight = height;
                }
            }
        }
        
        score += (myMaxHeight - oppMaxHeight) * 50;
        
        return score;
    }
}
