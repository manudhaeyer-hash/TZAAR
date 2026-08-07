import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.File;
import javax.imageio.ImageIO;

public class GenerateLogo {
    public static void main(String[] args) throws Exception {
        int width = 800;
        int height = 600;
        BufferedImage img = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2 = img.createGraphics();
        
        g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g2.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
        g2.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BILINEAR);
        
        // Draw 3 stacks of tokens
        // We'll load the token images
        String assetPath = "c:/AGrav/TZAAR/src/main/resources/view/assets/";
        BufferedImage redTzaar = ImageIO.read(new File(assetPath + "red_tzaar.png"));
        BufferedImage redTzarra = ImageIO.read(new File(assetPath + "red_tzarra.png"));
        BufferedImage redTott = ImageIO.read(new File(assetPath + "red_tott.png"));
        BufferedImage blueTzaar = ImageIO.read(new File(assetPath + "blue_tzaar.png"));
        BufferedImage blueTott = ImageIO.read(new File(assetPath + "blue_tott.png"));
        
        int tokenSize = 100;
        int spacing = 30;
        int startX = (width - (3 * tokenSize + 2 * spacing)) / 2;
        int stackY = 150;
        int yOffset = -25; // How much a token overlaps the one below it
        
        // Stack 1 (Left)
        g2.drawImage(redTzarra, startX, stackY, tokenSize, tokenSize, null);
        g2.drawImage(redTzaar, startX, stackY + yOffset, tokenSize, tokenSize, null);
        
        // Stack 2 (Middle)
        g2.drawImage(blueTott, startX + tokenSize + spacing, stackY, tokenSize, tokenSize, null);
        g2.drawImage(blueTott, startX + tokenSize + spacing, stackY + yOffset, tokenSize, tokenSize, null);
        g2.drawImage(blueTzaar, startX + tokenSize + spacing, stackY + yOffset * 2, tokenSize, tokenSize, null);
        
        // Stack 3 (Right)
        g2.drawImage(redTott, startX + 2 * (tokenSize + spacing), stackY, tokenSize, tokenSize, null);
        
        // Draw Title "TZAAR"
        g2.setColor(new Color(220, 220, 230)); // slightly metallic white
        Font titleFont = new Font("SansSerif", Font.BOLD, 120);
        g2.setFont(titleFont);
        
        FontMetrics fm = g2.getFontMetrics();
        String title = "TZAAR";
        int titleWidth = fm.stringWidth(title);
        int titleX = (width - titleWidth) / 2;
        int titleY = 400;
        
        // Add a slight drop shadow to title
        g2.setColor(new Color(0, 0, 0, 150));
        g2.drawString(title, titleX + 5, titleY + 5);
        g2.setColor(new Color(240, 240, 250));
        g2.drawString(title, titleX, titleY);
        
        // Draw Subtitle
        Font subFont = new Font("SansSerif", Font.PLAIN, 36);
        g2.setFont(subFont);
        fm = g2.getFontMetrics();
        String sub = "An abstract strategy game";
        int subWidth = fm.stringWidth(sub);
        int subX = (width - subWidth) / 2;
        int subY = 480;
        
        g2.setColor(new Color(0, 0, 0, 150));
        g2.drawString(sub, subX + 3, subY + 3);
        g2.setColor(new Color(180, 180, 190));
        g2.drawString(sub, subX, subY);
        
        g2.dispose();
        
        // Save
        ImageIO.write(img, "png", new File(assetPath + "logo2.png"));
        System.out.println("Logo 2 generated successfully.");
    }
}
