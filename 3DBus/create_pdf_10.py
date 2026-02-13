from fpdf import FPDF
import os

# Configuration: List of 10 specific images with their captions
# I have selected the best distinct shots from your uploads.
images_to_pdf = [
    ("Screenshot (189).png", "Fig 1: Front View of the Bus"),
    ("Screenshot (203).png", "Fig 2: Rear View of the Bus"),
    ("Screenshot (191).png", "Fig 3: Left Side View"),
    ("Screenshot (204).png", "Fig 4: Right Side View"),
    ("Screenshot (199).png", "Fig 5: Bird's Eye View (Top-Down Camera Mode)"),
    ("Screenshot (192).png", "Fig 6: Yaw Rotation (Camera Angled Left/Right)"),
    ("Screenshot (193).png", "Fig 7: Pitch Rotation (Camera Looking Down - X Axis)"),
    ("Screenshot (195).png", "Fig 8: Roll Rotation (Camera Tilted - Z Axis)"),
    ("Screenshot (201).png", "Fig 9: Isometric Front Perspective"),
    ("Screenshot (197).png", "Fig 10: Isometric Rear Perspective")
]

class PDF(FPDF):
    def footer(self):
        # No footer, page numbers, or text
        pass

    def header(self):
        # No header or logos
        pass

# Create PDF object (Portrait, A4)
pdf = PDF()
pdf.set_auto_page_break(auto=True, margin=15)

# Loop through the 10 images
for image_file, caption in images_to_pdf:
    if os.path.exists(image_file):
        pdf.add_page()
        
        # 1. Add Image
        # A4 width is 210mm. We use 190mm width (10mm margin left/right).
        # We place it slightly down (y=20) to look centered vertically.
        pdf.image(image_file, x=10, y=30, w=190)
        
        # 2. Add Caption
        # Position cursor below the image (approx y=150 depending on aspect ratio)
        pdf.set_y(150)
        pdf.set_font("Arial", size=14) # Slightly larger font for visibility
        pdf.cell(0, 10, txt=caption, ln=True, align='C')
    else:
        print(f"Skipping {image_file} (Not found in folder)")

# Output the file
output_filename = "Bus_Assignment_10_Images.pdf"
pdf.output(output_filename)

print(f"Done! Created: {output_filename}")