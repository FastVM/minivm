local sqrt = math.sqrt
local PI = 3.141592653589793
local SOLAR_MASS = 4 * PI * PI
local DAYS_PER_YEAR = 365.24
local bodies = {
    {x=0, y=0, z=0, vx=0, vy=0, vz=0, mass=SOLAR_MASS},
    {x=4.84143144246472090, y=-1.16032004402742839, z=-0.103622044471123109,
     vx=0.00166007664274403694*DAYS_PER_YEAR, vy=0.00769901118419740425*DAYS_PER_YEAR, vz=-0.0000690460016972063023*DAYS_PER_YEAR, mass=0.000954791938424326609*SOLAR_MASS},
    {x=8.34336671824457987, y=4.12479856412430479, z=-0.403523417114321381,
     vx=-0.00276742510726862411*DAYS_PER_YEAR, vy=0.00499852801234917238*DAYS_PER_YEAR, vz=0.0000230417297573763929*DAYS_PER_YEAR, mass=0.000285885980666130812*SOLAR_MASS},
    {x=12.8943695621391310, y=-15.1111514016986312, z=-0.223307578892655734,
     vx=0.00296460137564761618*DAYS_PER_YEAR, vy=0.00237847173959480950*DAYS_PER_YEAR, vz=-0.0000296589568540237556*DAYS_PER_YEAR, mass=0.0000436624404335156298*SOLAR_MASS},
    {x=15.3796971148509165, y=-25.9193146099879641, z=0.179258772950371181,
     vx=0.00268067772490389322*DAYS_PER_YEAR, vy=0.00162824170038242295*DAYS_PER_YEAR, vz=-0.0000951592254519715870*DAYS_PER_YEAR, mass=0.0000515138902046611451*SOLAR_MASS},
}
local function advance(dt)
    for i = 1, 5 do
        local bi = bodies[i]
        for j = i + 1, 5 do
            local bj = bodies[j]
            local dx, dy, dz = bi.x - bj.x, bi.y - bj.y, bi.z - bj.z
            local dist = sqrt(dx*dx + dy*dy + dz*dz)
            local mag = dt / (dist * dist * dist)
            local bjm, bim = bj.mass * mag, bi.mass * mag
            bi.vx = bi.vx - dx * bjm
            bi.vy = bi.vy - dy * bjm
            bi.vz = bi.vz - dz * bjm
            bj.vx = bj.vx + dx * bim
            bj.vy = bj.vy + dy * bim
            bj.vz = bj.vz + dz * bim
        end
    end
    for i = 1, 5 do
        local b = bodies[i]
        b.x = b.x + dt * b.vx
        b.y = b.y + dt * b.vy
        b.z = b.z + dt * b.vz
    end
end
local function energy()
    local e = 0
    for i = 1, 5 do
        local bi = bodies[i]
        e = e + 0.5 * bi.mass * (bi.vx*bi.vx + bi.vy*bi.vy + bi.vz*bi.vz)
        for j = i + 1, 5 do
            local bj = bodies[j]
            local dx, dy, dz = bi.x - bj.x, bi.y - bj.y, bi.z - bj.z
            e = e - (bi.mass * bj.mass) / sqrt(dx*dx + dy*dy + dz*dz)
        end
    end
    return e
end
local px, py, pz = 0, 0, 0
for i = 1, 5 do
    local b = bodies[i]
    px = px + b.vx * b.mass
    py = py + b.vy * b.mass
    pz = pz + b.vz * b.mass
end
local sun = bodies[1]
sun.vx = -px / SOLAR_MASS
sun.vy = -py / SOLAR_MASS
sun.vz = -pz / SOLAR_MASS
print(string.format("%0.9f", energy()))
for i = 1, 1000000 do
    advance(0.01)
end
print(string.format("%0.9f", energy()))
