"""
Mawaqit Display Simulator
Generates screenshots of the ESP32 display for documentation
"""

from PIL import Image, ImageDraw, ImageFont
import math
import os

# Display dimensions
WIDTH = 480
HEIGHT = 272

# Theme colors (RGB)
THEMES = {
    'green': {
        'bg': (0, 16, 0),
        'header': (0, 32, 0),
        'accent': (0, 255, 0),
        'text': (255, 255, 255)
    },
    'blue': {
        'bg': (0, 0, 32),
        'header': (24, 48, 96),
        'accent': (90, 180, 255),
        'text': (255, 255, 255)
    },
    'purple': {
        'bg': (32, 0, 32),
        'header': (64, 0, 64),
        'accent': (180, 120, 255),
        'text': (255, 255, 255)
    },
    'dark': {
        'bg': (0, 0, 0),
        'header': (24, 24, 24),
        'accent': (80, 80, 80),
        'text': (200, 200, 200)
    }
}

# Prayer data
PRAYERS = [
    ('Fajr', '05:47'),
    ('Shuruk', '07:32'),
    ('Dhuhr', '12:36'),
    ('Asr', '14:58'),
    ('Maghrib', '17:23'),
    ('Isha', '18:53')
]

def create_prayer_list(theme_name='green', current_time='14:30'):
    """Generate prayer list view"""
    theme = THEMES[theme_name]
    img = Image.new('RGB', (WIDTH, HEIGHT), theme['bg'])
    draw = ImageDraw.Draw(img)
    
    # Try to load font, fall back to default
    try:
        font_large = ImageFont.truetype("arial.ttf", 24)
        font_medium = ImageFont.truetype("arial.ttf", 18)
        font_small = ImageFont.truetype("arial.ttf", 14)
    except:
        font_large = ImageFont.load_default()
        font_medium = ImageFont.load_default()
        font_small = ImageFont.load_default()
    
    # Header bar
    draw.rectangle([0, 0, WIDTH, 40], fill=theme['header'])
    draw.line([0, 40, WIDTH, 40], fill=theme['accent'], width=2)
    
    # Current time in header
    draw.text((10, 10), current_time, fill=(255, 255, 0), font=font_large)
    
    # Mosque name
    draw.text((WIDTH - 200, 12), "Moschee Hamburg", fill=theme['text'], font=font_medium)
    
    # Prayer list - 2 columns
    col_width = WIDTH // 2 - 15
    row_height = 35
    start_y = 50
    
    current_hour = int(current_time.split(':')[0])
    current_min = int(current_time.split(':')[1])
    current_mins = current_hour * 60 + current_min
    
    # Find next prayer
    next_idx = 0
    for i, (name, time) in enumerate(PRAYERS):
        h, m = int(time.split(':')[0]), int(time.split(':')[1])
        if h * 60 + m > current_mins:
            next_idx = i
            break
    
    for i, (name, time) in enumerate(PRAYERS):
        col = 0 if i < 3 else 1
        row = i % 3
        x = 10 + col * (col_width + 15)
        y = start_y + row * (row_height + 8)
        
        # Determine prayer status
        h, m = int(time.split(':')[0]), int(time.split(':')[1])
        prayer_mins = h * 60 + m
        is_next = (i == next_idx)
        is_passed = prayer_mins < current_mins
        is_sunrise = (i == 1)
        
        # Background
        if is_next:
            draw.rounded_rectangle([x, y, x + col_width, y + row_height], 
                                  radius=6, fill=theme['accent'])
            # Accent bar
            draw.rectangle([x, y, x + 4, y + row_height], fill=theme['text'])
            text_color = (0, 0, 0)
        elif is_passed:
            draw.rounded_rectangle([x, y, x + col_width, y + row_height], 
                                  radius=6, fill=(30, 30, 30))
            text_color = (100, 100, 100)
        elif is_sunrise:
            draw.rounded_rectangle([x, y, x + col_width, y + row_height], 
                                  radius=6, fill=(60, 40, 10))
            text_color = (255, 165, 0)
        else:
            draw.rounded_rectangle([x, y, x + col_width, y + row_height], 
                                  radius=6, fill=theme['header'])
            text_color = theme['text']
        
        # Prayer name and time
        draw.text((x + 12, y + 8), name, fill=text_color, font=font_medium)
        draw.text((x + col_width - 60, y + 8), time, fill=text_color, font=font_medium)
    
    # Bottom info bar
    draw.rectangle([0, HEIGHT - 28, WIDTH, HEIGHT], fill=theme['header'])
    draw.line([0, HEIGHT - 28, WIDTH, HEIGHT - 28], fill=theme['accent'], width=1)
    
    # Next prayer info
    next_name, next_time = PRAYERS[next_idx]
    nh, nm = int(next_time.split(':')[0]), int(next_time.split(':')[1])
    diff = (nh * 60 + nm) - current_mins
    info = f"Next: {next_name} in {diff // 60}:{diff % 60:02d}"
    draw.text((10, HEIGHT - 20), info, fill=theme['text'], font=font_small)
    
    # Tap hint
    draw.text((WIDTH - 100, HEIGHT - 20), "Tap: Modes", fill=theme['accent'], font=font_small)
    
    return img


def create_clock_view(theme_name='green', current_time='14:30'):
    """Generate analog clock view"""
    theme = THEMES[theme_name]
    img = Image.new('RGB', (WIDTH, HEIGHT), theme['bg'])
    draw = ImageDraw.Draw(img)
    
    try:
        font_large = ImageFont.truetype("arial.ttf", 28)
        font_medium = ImageFont.truetype("arial.ttf", 18)
        font_small = ImageFont.truetype("arial.ttf", 12)
    except:
        font_large = ImageFont.load_default()
        font_medium = ImageFont.load_default()
        font_small = ImageFont.load_default()
    
    # Clock center and radius
    cx, cy = 130, HEIGHT // 2
    r = 100
    
    # Clock face
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], outline=theme['accent'], width=3)
    draw.ellipse([cx - r + 5, cy - r + 5, cx + r - 5, cy + r - 5], fill=theme['header'])
    
    # Hour marks
    for i in range(12):
        angle = math.radians(i * 30 - 90)
        x1 = cx + (r - 15) * math.cos(angle)
        y1 = cy + (r - 15) * math.sin(angle)
        x2 = cx + (r - 8) * math.cos(angle)
        y2 = cy + (r - 8) * math.sin(angle)
        draw.line([x1, y1, x2, y2], fill=theme['text'], width=2)
    
    # Parse time
    h = int(current_time.split(':')[0])
    m = int(current_time.split(':')[1])
    s = 30  # Simulated seconds
    
    # Hour hand
    hour_angle = math.radians((h % 12 + m / 60) * 30 - 90)
    hx = cx + 50 * math.cos(hour_angle)
    hy = cy + 50 * math.sin(hour_angle)
    draw.line([cx, cy, hx, hy], fill=theme['text'], width=4)
    
    # Minute hand
    min_angle = math.radians(m * 6 - 90)
    mx = cx + 70 * math.cos(min_angle)
    my = cy + 70 * math.sin(min_angle)
    draw.line([cx, cy, mx, my], fill=theme['accent'], width=3)
    
    # Second hand
    sec_angle = math.radians(s * 6 - 90)
    sx = cx + 80 * math.cos(sec_angle)
    sy = cy + 80 * math.sin(sec_angle)
    draw.line([cx, cy, sx, sy], fill=(255, 255, 0), width=1)
    
    # Center dot
    draw.ellipse([cx - 6, cy - 6, cx + 6, cy + 6], fill=theme['accent'])
    
    # Right side - Digital time
    rx = 260
    draw.text((rx, 8), current_time, fill=theme['text'], font=font_large)
    draw.text((rx + 90, 15), f":{s:02d}", fill=theme['accent'], font=font_medium)
    
    # Date
    draw.text((rx, 45), "Mon, 28. Jan", fill=theme['text'], font=font_small)
    
    # Prayer times list
    list_y = 65
    row_h = 30
    
    current_mins = h * 60 + m
    next_idx = 0
    for i, (name, time) in enumerate(PRAYERS):
        ph, pm = int(time.split(':')[0]), int(time.split(':')[1])
        if ph * 60 + pm > current_mins:
            next_idx = i
            break
    
    for i, (name, time) in enumerate(PRAYERS):
        y = list_y + i * row_h
        is_next = (i == next_idx)
        
        if is_next:
            draw.rounded_rectangle([rx - 5, y, rx + 200, y + row_h - 4], 
                                  radius=4, fill=theme['header'])
        
        ph, pm = int(time.split(':')[0]), int(time.split(':')[1])
        passed = ph * 60 + pm <= current_mins and not is_next
        
        color = (100, 100, 100) if passed else (theme['accent'] if is_next else theme['text'])
        if i == 1:  # Sunrise
            color = (255, 165, 0) if not passed else (100, 70, 0)
        
        draw.text((rx, y + 3), name[:5], fill=color, font=font_medium)
        draw.text((rx + 70, y + 3), time, fill=color, font=font_medium)
        
        if is_next:
            diff = (ph * 60 + pm) - current_mins
            draw.text((rx + 140, y + 3), f"{diff // 60}h{diff % 60:02d}", 
                     fill=theme['accent'], font=font_medium)
    
    return img


def create_countdown_view(theme_name='green', current_time='14:30'):
    """Generate countdown view"""
    theme = THEMES[theme_name]
    img = Image.new('RGB', (WIDTH, HEIGHT), theme['bg'])
    draw = ImageDraw.Draw(img)
    
    try:
        font_huge = ImageFont.truetype("arial.ttf", 48)
        font_large = ImageFont.truetype("arial.ttf", 32)
        font_medium = ImageFont.truetype("arial.ttf", 20)
        font_small = ImageFont.truetype("arial.ttf", 14)
    except:
        font_huge = ImageFont.load_default()
        font_large = ImageFont.load_default()
        font_medium = ImageFont.load_default()
        font_small = ImageFont.load_default()
    
    h = int(current_time.split(':')[0])
    m = int(current_time.split(':')[1])
    current_mins = h * 60 + m
    
    # Find next prayer
    next_name = "Fajr"
    next_time = "05:47"
    diff_mins = 0
    
    for name, time in PRAYERS:
        if name == "Shuruk":
            continue
        ph, pm = int(time.split(':')[0]), int(time.split(':')[1])
        if ph * 60 + pm > current_mins:
            next_name = name
            next_time = time
            diff_mins = (ph * 60 + pm) - current_mins
            break
    
    # "Next" label
    text = "NEXT"
    bbox = draw.textbbox((0, 0), text, font=font_medium)
    text_width = bbox[2] - bbox[0]
    draw.text(((WIDTH - text_width) // 2, 20), text, fill=(150, 150, 150), font=font_medium)
    
    # Prayer name - big
    bbox = draw.textbbox((0, 0), next_name, font=font_huge)
    text_width = bbox[2] - bbox[0]
    draw.text(((WIDTH - text_width) // 2, 55), next_name, fill=theme['accent'], font=font_huge)
    
    # Time
    bbox = draw.textbbox((0, 0), next_time, font=font_large)
    text_width = bbox[2] - bbox[0]
    draw.text(((WIDTH - text_width) // 2, 115), next_time, fill=theme['text'], font=font_large)
    
    # Countdown box
    box_w = 200
    box_h = 60
    box_x = (WIDTH - box_w) // 2
    box_y = 155
    draw.rounded_rectangle([box_x, box_y, box_x + box_w, box_y + box_h], 
                          radius=10, fill=theme['header'], outline=theme['accent'], width=2)
    
    # Countdown time
    diff_h = diff_mins // 60
    diff_m = diff_mins % 60
    countdown = f"{diff_h}:{diff_m:02d}:30"
    bbox = draw.textbbox((0, 0), countdown, font=font_large)
    text_width = bbox[2] - bbox[0]
    draw.text(((WIDTH - text_width) // 2, box_y + 12), countdown, fill=theme['text'], font=font_large)
    
    # "remaining" label
    bbox = draw.textbbox((0, 0), "remaining", font=font_small)
    text_width = bbox[2] - bbox[0]
    draw.text(((WIDTH - text_width) // 2, box_y + box_h + 5), "remaining", 
             fill=(150, 150, 150), font=font_small)
    
    # Current time at bottom
    draw.text((10, HEIGHT - 25), current_time + ":30", fill=(80, 80, 80), font=font_medium)
    
    # Mosque name
    draw.text((WIDTH - 150, HEIGHT - 20), "Moschee Hamburg", fill=(80, 80, 80), font=font_small)
    
    return img


def main():
    """Generate all screenshots"""
    output_dir = "screenshots"
    os.makedirs(output_dir, exist_ok=True)
    
    # Generate for each view and theme
    views = [
        ('list', create_prayer_list),
        ('clock', create_clock_view),
        ('countdown', create_countdown_view)
    ]
    
    for view_name, view_func in views:
        for theme_name in THEMES.keys():
            img = view_func(theme_name, '14:30')
            filename = f"{output_dir}/display_{view_name}_{theme_name}.png"
            img.save(filename)
            print(f"Created: {filename}")
    
    # Create main showcase image (green theme, list view)
    img = create_prayer_list('green', '14:30')
    img.save(f"{output_dir}/display_main.png")
    print(f"Created: {output_dir}/display_main.png")
    
    # Create combined showcase
    combined = Image.new('RGB', (WIDTH * 3 + 20, HEIGHT + 40), (30, 30, 30))
    
    list_img = create_prayer_list('green', '14:30')
    clock_img = create_clock_view('green', '14:30')
    countdown_img = create_countdown_view('green', '14:30')
    
    combined.paste(list_img, (5, 30))
    combined.paste(clock_img, (WIDTH + 10, 30))
    combined.paste(countdown_img, (WIDTH * 2 + 15, 30))
    
    draw = ImageDraw.Draw(combined)
    try:
        font = ImageFont.truetype("arial.ttf", 16)
    except:
        font = ImageFont.load_default()
    
    draw.text((WIDTH // 2 - 30, 8), "List View", fill=(200, 200, 200), font=font)
    draw.text((WIDTH + WIDTH // 2 - 30, 8), "Clock View", fill=(200, 200, 200), font=font)
    draw.text((WIDTH * 2 + WIDTH // 2 - 40, 8), "Countdown", fill=(200, 200, 200), font=font)
    
    combined.save(f"{output_dir}/display_showcase.png")
    print(f"Created: {output_dir}/display_showcase.png")


if __name__ == "__main__":
    main()
