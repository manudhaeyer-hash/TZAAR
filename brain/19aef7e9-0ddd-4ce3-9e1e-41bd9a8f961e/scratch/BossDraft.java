import java.util.*;

public class BossDraft {
    
    static final int WALL = -1;
    static final int EMPTY = 0;
    static final int[] DIRS = {16, 1, -15, -16, -1, 15};
    
    static int makePiece(int owner, int type, int height) {
        return (1 << 11) | (owner << 10) | (type << 8) | height;
    }
    static int getOwner(int p) { return (p >> 10) & 1; }
    static int getType(int p) { return (p >> 8) & 3; }
    static int getHeight(int p) { return p & 0xFF; }

    static class State {
        int[] board = new int[256];
        int myId;
        int phase; // 1 or 2
        
        // Track piece counts for quick evaluation
        // counts[owner][type]
        int[][] typeCounts = new int[2][3];
        
        // List of piece locations for each player
        // To iterate quickly over our pieces
        int[] myPieces = new int[60];
        int myPiecesCount = 0;
        
        int[] oppPieces = new int[60];
        int oppPiecesCount = 0;
        
        void initBoard() {
            Arrays.fill(board, WALL);
            for (int q = 0; q <= 8; q++) {
                for (int r = 0; r <= 8; r++) {
                    int s = - (q-4) - (r-4);
                    if (s >= -4 && s <= 4) {
                        if (!(q==4 && r==4)) { // Center is missing
                            board[q * 16 + r] = EMPTY;
                        }
                    }
                }
            }
        }
        
        void clearPieces() {
            myPiecesCount = 0;
            oppPiecesCount = 0;
            for(int i=0; i<2; i++) {
                for(int j=0; j<3; j++) {
                    typeCounts[i][j] = 0;
                }
            }
            for (int i = 0; i < 256; i++) {
                if (board[i] > 0) board[i] = EMPTY;
            }
        }
        
        void addPiece(int q, int r, int owner, int type, int height) {
            int idx = q * 16 + r;
            int p = makePiece(owner, type, height);
            board[idx] = p;
            typeCounts[owner][type]++;
            if (owner == myId) {
                myPieces[myPiecesCount++] = idx;
            } else {
                oppPieces[oppPiecesCount++] = idx;
            }
        }
    }
    
    // Generates moves as int: (from << 8) | to
    static int generateMoves(State st, int[] moves, int playerToMove) {
        int moveCount = 0;
        
        int[] pieces = (playerToMove == st.myId) ? st.myPieces : st.oppPieces;
        int count = (playerToMove == st.myId) ? st.myPiecesCount : st.oppPiecesCount;
        
        for (int i = 0; i < count; i++) {
            int from = pieces[i];
            int p = st.board[from];
            if (p <= 0 || getOwner(p) != playerToMove) continue; // In case piece was captured in search
            
            int height = getHeight(p);
            
            for (int d : DIRS) {
                int curr = from + d;
                while (st.board[curr] == EMPTY) {
                    curr += d;
                }
                
                int target = st.board[curr];
                if (target > 0) {
                    int targetOwner = getOwner(target);
                    int targetHeight = getHeight(target);
                    
                    if (st.phase == 1) {
                        // Phase 1: MUST capture
                        if (targetOwner != playerToMove && height >= targetHeight) {
                            moves[moveCount++] = (from << 8) | curr;
                        }
                    } else {
                        // Phase 2: Capture or Reinforce
                        if (targetOwner == playerToMove) {
                            // Reinforce
                            moves[moveCount++] = (from << 8) | curr;
                        } else {
                            // Capture
                            if (height >= targetHeight) {
                                moves[moveCount++] = (from << 8) | curr;
                            }
                        }
                    }
                }
            }
        }
        
        return moveCount;
    }
    
    // Evaluation:
    // +INF if opp loses (0 of a type)
    // -INF if I lose (0 of a type)
    // Material advantage
    static int evaluate(State st) {
        // Check win/loss condition
        boolean iLost = false;
        boolean oppLost = false;
        
        for (int t = 0; t < 3; t++) {
            if (st.typeCounts[st.myId][t] == 0) iLost = true;
            if (st.typeCounts[1 - st.myId][t] == 0) oppLost = true;
        }
        
        if (iLost && !oppLost) return -1000000;
        if (oppLost && !iLost) return 1000000;
        if (iLost && oppLost) return 0; // Draw? Should not happen in turn-based, but just in case
        
        int score = 0;
        
        int oppId = 1 - st.myId;
        
        // Type weight: TZAAR=3000, TZARRA=2000, TOTT=1000
        int[] typeWeights = {1000, 2000, 3000};
        
        for (int t = 0; t < 3; t++) {
            int myCount = st.typeCounts[st.myId][t];
            int oppCount = st.typeCounts[oppId][t];
            
            score += (myCount - oppCount) * typeWeights[t];
            
            // Vulnerability penalty
            if (myCount == 1) score -= 15000;
            if (oppCount == 1) score += 15000;
        }
        
        // TODO: height evaluation and mobility
        
        return score;
    }
}
