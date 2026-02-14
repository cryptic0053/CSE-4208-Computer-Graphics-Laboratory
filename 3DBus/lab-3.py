from fpdf import FPDF
import os

# File paths based on user uploads
image_files = {
    '222': 'Screenshot (222).png',  # Directional
    '224': 'Screenshot (224).png',  # Point
    '219': 'Screenshot (219).png',  # Spot
    '223': 'Screenshot (223).png',  # Ambient
    '221': 'Screenshot (221).png',  # Diffuse
    '220': 'Screenshot (220).png',  # Specular
    '218': 'Screenshot (218).png'   # Combined
}

# Mapping images to captions and pages
# Format: (filename_key, caption)
page1_content = [
    ('222', 'Fig 1: Directional Light (Sunlight effect)'),
    ('224', 'Fig 2: Point Lights (Warm/Local illumination)'),
    ('219', 'Fig 3: Spot Light (Cone cut-off angle)'),
    ('223', 'Fig 4: Ambient Light Only')
]

page2_content = [
    ('221', 'Fig 5: Diffuse Light Component'),
    ('220', 'Fig 6: Specular Light Component'),
    ('218', 'Fig 7: Combined Lighting Implementation')
]

class PDF(FPDF):
    def header(self):
        self.set_font('Arial', 'B', 15)
        self.cell(0, 10, 'Assignment: Lighting Implementation', 0, 1, 'C')
        self.ln(5)

    def footer(self):
        self.set_y(-15)
        self.set_font('Arial', 'I', 8)
        self.cell(0, 10, f'Page {self.page_no()}', 0, 0, 'C')

    def add_image_grid(self, content_list):
        self.set_font('Arial', '', 10)
        
        # Grid settings
        x_start = 10
        y_start = 30
        img_width = 90
        img_height = 50 # Approximate aspect ratio
        h_gap = 5
        v_gap = 20
        
        for i, (key, caption) in enumerate(content_list):
            # Calculate position (2 columns)
            col = i % 2
            row = i // 2
            
            x = x_start + (col * (img_width + h_gap))
            y = y_start + (row * (img_height + v_gap))
            
            # Place Image
            if key in image_files:
                self.image(image_files[key], x=x, y=y, w=img_width)
                
            # Place Caption
            self.set_xy(x, y + img_height + 2)
            self.multi_cell(img_width, 5, caption, 0, 'C')

pdf = PDF()
pdf.set_auto_page_break(auto=True, margin=15)

# Page 1
pdf.add_page()
pdf.add_image_grid(page1_content)

# Page 2
pdf.add_page()
pdf.add_image_grid(page2_content)

output_filename = "Lighting_Assignment_Report.pdf"
pdf.output(output_filename)