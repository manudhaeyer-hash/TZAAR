import java.util.*;

class Player {
    public static void main(String[] args) {
        Scanner in = new Scanner(System.in);
        int myId = in.nextInt();
        
        while (true) {
            int phase = in.nextInt();
            int pieceCount = in.nextInt();
            for (int i = 0; i < pieceCount; i++) {
                int x = in.nextInt();
                int y = in.nextInt();
                int owner = in.nextInt();
                int type = in.nextInt();
                int height = in.nextInt();
            }
            
            // TODO: Implement your bot here!
            System.out.println("WAIT");
        }
    }
}
