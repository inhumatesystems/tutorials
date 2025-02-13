
import inhumate_rti as RTI
import pygame
import math
import random

# Set up the game window
pygame.init()
screen = pygame.display.set_mode((1000, 1000))
clock = pygame.time.Clock()

# Set up the RTI connection
rti = RTI.Client(application="SimpleSim")
rti.wait_until_connected()

# Player settings
player_speed = 3
player_rotation_speed = 3

# Player state
player_x = random.randint(10, 990)
player_y = random.randint(10, 990)
player_heading = random.randint(0, 360)

# Publish player entity
entity = RTI.proto.Entity()
entity.id = "player" + str(random.randint(0, 9999))
entity.type = "player"
rti.publish(RTI.channel.entity, entity)

# Publish entity update on request
def on_entity_operation(operation):
    if operation.request_update:
        rti.publish(RTI.channel.entity, entity)
rti.subscribe(RTI.channel.entity_operation, RTI.proto.EntityOperation, on_entity_operation)

try:
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT: running = False
    
        # Moving the player
        keys = pygame.key.get_pressed()
        if keys[pygame.K_a]: # rotate left
            player_heading -= player_rotation_speed
        if keys[pygame.K_d]: # rotate right
            player_heading += player_rotation_speed
        if keys[pygame.K_w]: # move forward
            player_x += math.sin(player_heading * math.pi / 180) * player_speed
            player_y -= math.cos(player_heading * math.pi / 180) * player_speed
        if keys[pygame.K_s]: # move backward
            player_x -= math.sin(player_heading * math.pi / 180) * player_speed
            player_y += math.cos(player_heading * math.pi / 180) * player_speed

        # Limit movement
        if player_heading < 0: player_heading += 360
        if player_heading > 360: player_heading -= 360
        if player_x < 10: player_x = 10
        if player_x > 990: player_x = 990
        if player_y < 10: player_y = 10
        if player_y > 990: player_y = 990
    
        # Render the screen
        screen.fill((0, 0, 255))
        pygame.draw.circle(screen, (255, 0, 0), (player_x, player_y), 10)
        pygame.draw.line(screen, (255, 0, 0), (player_x, player_y), 
                        (player_x + math.sin(player_heading * math.pi / 180) * 15, player_y - math.cos(player_heading * math.pi / 180) * 15), 5)
        pygame.display.update()

        # Publish player position
        position = RTI.proto.EntityPosition()
        position.id = entity.id
        position.local.x = player_x - 500
        position.local.z = 500 - player_y
        position.euler_rotation.yaw = player_heading
        position.geodetic.latitude = 59.36 + (500 - player_y) / 111320
        position.geodetic.longitude = 17.96 + (player_x - 500) / 56733
        rti.publish(RTI.channel.position, position)

        # Ensure a stable frame rate
        clock.tick(30)
finally:
    entity.deleted = True
    rti.publish(RTI.channel.entity, entity)
    pygame.quit()
