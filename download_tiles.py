import urllib.request
import os
import tarfile
import math

# USGS Imagery Topo (Satellite + Detailed Roads/City labels)
URL_TEMPLATE = 'https://basemap.nationalmap.gov/arcgis/rest/services/USGSImageryTopo/MapServer/tile/{z}/{y}/{x}'

def deg2num(lat_deg, lon_deg, zoom):
    lat_rad = math.radians(lat_deg)
    n = 2.0 ** zoom
    xtile = int((lon_deg + 180.0) / 360.0 * n)
    ytile = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return (xtile, ytile)

def download_tiles():
    if not os.path.exists('tiles'):
        os.makedirs('tiles')
        
    # USA Bounding Box (Roughly)
    min_lat, max_lat = 24.0, 49.0
    min_lon, max_lon = -125.0, -66.0

    with tarfile.open('USGSImageryTopo.tar', 'w') as tar:
        # Download Global (Low res)
        for z in range(0, 5):
            for x in range(2**z):
                for y in range(2**z):
                    fetch_and_tar(tar, z, y, x)
        
        # Download USA (High-res up to zoom 14)
        for z in range(5, 12):
            x_min, y_max = deg2num(min_lat, min_lon, z)
            x_max, y_min = deg2num(max_lat, max_lon, z)
            for x in range(x_min, x_max + 1):
                for y in range(y_min, y_max + 1):
                    fetch_and_tar(tar, z, y, x)

def fetch_and_tar(tar, z, y, x):
    url = URL_TEMPLATE.format(z=z, y=y, x=x)
    num_size = len(str(int(2**z)))
    # Match the C++ string padding logic exactly!
    filename = f'z{z:02d}y{y:0{num_size}d}x{x:0{num_size}d}.jpg'
    filepath = os.path.join('tiles', filename)
    if not os.path.exists(filepath):
        try:
            req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(req) as response, open(filepath, 'wb') as out_file:
                out_file.write(response.read())
            tar.add(filepath, arcname=filename)
            print(f'Downloaded {filename}')
        except Exception as e:
            pass

download_tiles()


